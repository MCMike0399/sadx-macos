"""Extract source-bound baseline evidence and Emerald Coast 1 geometry.

The mesh is a rendering bring-up artifact, not a reconstructed gameplay scene.
Animation, textures, UVs, material effects, visibility/collision semantics and
object placement from SET files are not implemented here.
"""
import argparse
import collections
import hashlib
import json
import math
import pathlib
import struct
import sys,pathlib as _pl;sys.path[:0]=[str(_pl.Path(__file__).resolve().parents[2]/'tools'/d) for d in('lib','oracles')]  # run from anywhere
from pe_image import PEImage

SOURCE_SHA256 = '6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
LAND_ADDRESS = 0xE99CB8
TABLE_ADDRESS = 0x88AF78
PARAMETER_ADDRESS = 0x9154E8
UNIT_MATRIX_ADDRESS = 0x389D650
INPUT_PRODUCER_CONSTANTS = ((0x7dccf4,4),(0x7dccf0,4),(0x7dcce0,4),(0x7dcb10,4),
                            (0x7dccd0,8),(0x7dccc8,8),(0x7dcce8,8))
GROUND_ACCELERATION_CONSTANTS = (
    0x7dcb2c,0x7dcb34,0x7dcc98,0x7dccfc,0x7dce5c,0x7dd540,
    0x7dd550,0x7dd568,0x7dd578,0x7dd584,0x7dfff0,0x7e0064,
    0x7e0068,0x7e0088,0x7e00e4,0x7e0124,0x7e0138,0x7e0148,
    0x7e014c,0x7e0178,0x7e019c,0x7e01a0,0x7e01a4,0x7e01a8)
PLAYER_ROTATION_CONSTANTS = (
    *((address,4) for address in (0x7dfff0,0x7dd568,0x7dd550,0x7e0070,0x7e0118,0x7dcc98,
        0x7dcb10,0x7e011c,0x7e0120,0x7dce68,0x7dcb2c,0x7e0164,0x7dccf4,0x7e0160,0x7e0170,0x7dcb60)),
    *((address,8) for address in (0x7e0168,0x7dccd0,0x7dcce8)))
PROJECTION_ANGLE_CONSTANTS = ((0x7dcb2c,4),(0x7dcb10,4),(0x7dcb0c,4),
                            (0x7dccd0,8),(0x7dccc8,8))
EVENT_ACCELERATION_CONSTANTS = ((0x7dcb10,4),(0x7dccd0,8),(0x7dccc8,8))
PLAYER_POSITION_CONSTANTS = ((0x7e012c,4),(0x7e01ac,4),(0x7dccd0,8),
                             (0x7dccc8,8),(0x7dcce8,8))


def identity():
    return [[float(i == j) for j in range(4)] for i in range(4)]


def multiply(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(4)) for j in range(4)] for i in range(4)]


def transform_point(m, p):
    return tuple(sum(m[i][j] * p[j] for j in range(3)) + m[i][3] for i in range(3))


def object_matrix(flags, position, angles, scale, trig):
    m = identity()
    if not flags & 1:
        for i in range(3):
            m[i][3] = position[i]
    if not flags & 2:
        # Ninja's default object rotation is Z,Y,X; ZXY flag chooses Z,X,Y.
        # Geometry bring-up only: matrix rounding is not original x87 execution.
        for axis in ([2, 0, 1] if flags & 0x20 else [2, 1, 0]):
            s = trig(angles[axis])
            c = trig(angles[axis] + 0x4000)
            r = identity()
            a, b = (axis + 1) % 3, (axis + 2) % 3
            r[a][a] = r[b][b] = c
            r[a][b], r[b][a] = -s, s
            m = multiply(m, r)
    if not flags & 4:
        s = identity()
        for i in range(3):
            s[i][i] = scale[i]
        m = multiply(m, s)
    return m


def sin_from_table(table, angle):
    angle &= 0xffff
    index, fraction = angle >> 4, angle & 15
    quadrant = index >> 10
    offset = index & 1023
    if quadrant & 1:
        first, second = table[1024 - offset], table[1023 - offset]
    else:
        first, second = table[offset], table[offset + 1]
    sign = -1 if quadrant >= 2 else 1
    return sign * (first + (second - first) * (fraction / 16))


def triangulate(kind, indices, reverse=False):
    if kind == 0:
        if len(indices) != 3:
            raise ValueError('Triangle requires 3 indices')
        return [tuple(indices)]
    if kind == 1:
        if len(indices) != 4:
            raise ValueError('Quad requires 4 indices')
        # Ninja quads are strips, not perimeter-ordered polygon fans.
        return [(indices[0], indices[1], indices[2]), (indices[2], indices[1], indices[3])]
    if kind == 2:
        raise ValueError('N polygon decoding requires producer evidence; unsupported')
    triangles = []
    for i in range(len(indices) - 2):
        a, b, c = indices[i:i + 3]
        triangles.append((b, a, c) if bool(i & 1) ^ reverse else (a, b, c))
    return triangles


def extract_stage(image, table):
    land = image.unpack('<4hf6I', LAND_ADDRESS)
    count, animated_count = land[:2]
    if count < 0 or animated_count != 0:
        raise ValueError('Unsupported land count or animated land geometry')
    cols = land[5]
    image.read(cols, 36 * count)
    output = bytearray()
    records = []
    mesh_types = collections.Counter()
    active = set()
    object_count = 0

    def visit(address, parent, source_nodes):
        nonlocal object_count
        siblings = []
        while address:
            if address in active:
                raise ValueError('Cyclic object graph')
            active.add(address)
            siblings.append(address)
            flags, model, *fields = image.unpack('<2I3f3i3f2I', address)
            position, angles, scale = fields[:3], fields[3:6], fields[6:9]
            child, sibling = fields[9:]
            if any(not math.isfinite(x) for x in position + scale):
                raise ValueError('Non-finite object transform')
            matrix = multiply(parent, object_matrix(flags, position, angles, scale,
                                                   lambda a: sin_from_table(table, a)))
            object_count += 1
            source_nodes.append(dict(address=hex(address), model=hex(model), flags=hex(flags),
                                     position=position, angles=angles, scale=scale))
            if model and not flags & 8:
                emit_model(model, matrix)
            if child and not flags & 16:
                visit(child, matrix, source_nodes)
            address = sibling
        for sibling in siblings:
            active.remove(sibling)

    def emit_model(address, matrix):
        points, _, point_count, meshes, materials, mesh_count, material_count, *_ = image.unpack('<2Ii2I2H4fI', address)
        if point_count < 0:
            raise ValueError('Negative point count')
        point_bytes = image.read(points, point_count * 12)
        vertices = [transform_point(matrix, p) for p in struct.iter_unpack('<3f', point_bytes)]
        if any(not math.isfinite(v) for p in vertices for v in p):
            raise ValueError('Non-finite model position')
        image.read(meshes, mesh_count * 28)
        image.read(materials, material_count * 20)
        for mi in range(mesh_count):
            type_material, polygon_count, indices_address, _, _, colors, _, _ = image.unpack('<2H6I', meshes + mi * 28)
            kind, material = type_material >> 14, type_material & 0x3fff
            if material >= material_count:
                raise ValueError('Material index exceeds model materials')
            color, = image.unpack('<I', materials + material * 20)
            mesh_types[kind] += 1
            cursor = indices_address
            color_cursor = colors
            for _ in range(polygon_count):
                reverse = False
                if kind < 2:
                    index_count = kind + 3
                else:
                    header, = image.unpack('<H', cursor)
                    cursor += 2
                    index_count = header & 0x3fff
                    reverse = bool(header & 0x8000)
                indices = image.unpack('<' + str(index_count) + 'H', cursor)
                cursor += index_count * 2
                if any(i >= point_count for i in indices):
                    raise ValueError('Vertex index exceeds point count')
                polygon_colors = image.unpack('<' + str(index_count) + 'I', color_cursor) if colors else [color] * index_count
                if colors:
                    color_cursor += index_count * 4
                for triangle in triangulate(kind, list(range(index_count)), reverse):
                    a, b, c = (vertices[indices[i]] for i in triangle)
                    if a == b or b == c or a == c:
                        continue
                    # Mild face lighting for untextured geometry inspection only.
                    u, v = [b[i] - a[i] for i in range(3)], [c[i] - a[i] for i in range(3)]
                    normal = (u[1]*v[2] - u[2]*v[1], u[2]*v[0] - u[0]*v[2], u[0]*v[1] - u[1]*v[0])
                    length = math.sqrt(sum(x*x for x in normal))
                    light = 0.45 + 0.55 * abs(normal[1] / length) if length else 1
                    for i in triangle:
                        p = vertices[indices[i]]
                        rgb = tuple(((polygon_colors[i] >> shift) & 255) / 255 * light for shift in (16, 8, 0))
                        output.extend(struct.pack('<8f', *p, 1, *rgb, 1))

    for i in range(count):
        center_and_radius = image.unpack('<6f3I', cols + i * 36)
        root, blockbits, flags = center_and_radius[6:]
        active.clear()
        nodes = []
        # Draw all available land meshes for extraction completeness. Runtime
        # COL visibility/solid flags remain attached in the report, not applied.
        visit(root, identity(), nodes)
        records.append(dict(ordinal=i, address=hex(cols + i * 36), root=hex(root),
                            blockbits=blockbits, flags=hex(flags), nodes=nodes))
    return bytes(output), dict(land_address=hex(LAND_ADDRESS), col_count=count,
                               object_count=object_count, vertex_count=len(output) // 32,
                               triangle_count=len(output) // 96,
                               mesh_types=dict(mesh_types), records=records)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('exe', type=pathlib.Path)
    parser.add_argument('--output', required=True, type=pathlib.Path)
    args = parser.parse_args()
    image = PEImage(args.exe)
    if image.sha256 != SOURCE_SHA256:
        raise ValueError('This address map requires the supplied executable SHA-256 ' + SOURCE_SHA256)
    args.output.mkdir(parents=True, exist_ok=True)
    table_bytes = image.read(TABLE_ADDRESS, 1025 * 4)
    table = struct.unpack('<1025f', table_bytes)
    mesh, stage = extract_stage(image, table)
    report = image.describe()
    report['evidence_kind'] = 'offline file-byte observations; not REA Evidence or runtime verification'
    report['stage'] = stage
    report['trig_table'] = dict(address=hex(TABLE_ADDRESS), file_offset=image.file_offset(TABLE_ADDRESS, len(table_bytes)),
                                size=len(table_bytes), sha256=hashlib.sha256(table_bytes).hexdigest())
    parameter_bytes = image.read(PARAMETER_ADDRESS, 8 * 132)
    report['player_parameters'] = dict(address=hex(PARAMETER_ADDRESS),
        file_offset=image.file_offset(PARAMETER_ADDRESS, len(parameter_bytes)),
        record_bytes=132, record_count=8,
        sha256=hashlib.sha256(parameter_bytes).hexdigest())
    unit_bytes = image.read(UNIT_MATRIX_ADDRESS,64)
    report['unit_matrix'] = dict(address=hex(UNIT_MATRIX_ADDRESS),
        file_offset=image.file_offset(UNIT_MATRIX_ADDRESS,64),
        bytes=64,sha256=hashlib.sha256(unit_bytes).hexdigest())
    input_constant_bytes = b''.join(image.read(address,size) for address,size in INPUT_PRODUCER_CONSTANTS)
    report['input_producer_constants'] = dict(bytes=len(input_constant_bytes),
        sha256=hashlib.sha256(input_constant_bytes).hexdigest(),
        fields=[dict(address=hex(address),size=size,file_offset=image.file_offset(address,size),
                     bytes=image.read(address,size).hex()) for address,size in INPUT_PRODUCER_CONSTANTS])
    ground_constant_bytes = b''.join(image.read(address,4) for address in GROUND_ACCELERATION_CONSTANTS)
    report['ground_acceleration_constants'] = dict(bytes=len(ground_constant_bytes),
        sha256=hashlib.sha256(ground_constant_bytes).hexdigest(),
        fields=[dict(address=hex(address),size=4,file_offset=image.file_offset(address,4),
                     bytes=image.read(address,4).hex()) for address in GROUND_ACCELERATION_CONSTANTS])
    rotation_constant_bytes = b''.join(image.read(address,size) for address,size in PLAYER_ROTATION_CONSTANTS)
    report['player_rotation_constants'] = dict(bytes=len(rotation_constant_bytes),
        sha256=hashlib.sha256(rotation_constant_bytes).hexdigest(),
        fields=[dict(address=hex(address),size=size,file_offset=image.file_offset(address,size),
                     bytes=image.read(address,size).hex()) for address,size in PLAYER_ROTATION_CONSTANTS])
    report['limitations'] = ['Untextured static land geometry only', 'Object transform rounding and rendering are reconstruction candidates',
                              'No playable gameplay, audio, menus, physics or controller mapping into game state',
                              'No speedrun validity established', 'COL flags retained but not interpreted for this inspection mesh']
    for name, address, size in [('CCEF-init', 0x434870, 10), ('CCEF-update', 0x438330, 10),
                                 ('EHCF-1', 0x533939, 3), ('EHCF-2', 0x440EF5, 2)]:
        report.setdefault('baseline_patch_sites', {})[name] = dict(address=hex(address),
                                                                  file_offset=image.file_offset(address, size),
                                                                  bytes=image.read(address, size).hex())
    (args.output / 'ninja-trig.f32').write_bytes(table_bytes)
    (args.output / 'player-parameters.bin').write_bytes(parameter_bytes)
    (args.output / 'ninja-unit.f32').write_bytes(unit_bytes)
    (args.output / 'input-producer-constants.bin').write_bytes(input_constant_bytes)
    (args.output / 'ground-acceleration-constants.bin').write_bytes(ground_constant_bytes)
    (args.output / 'player-rotation-constants.bin').write_bytes(rotation_constant_bytes)
    for name,ranges in (('projection-angle-constants',PROJECTION_ANGLE_CONSTANTS),
                        ('ninja-blend-tables',((0x88ad64,56),)),
                        ('ninja-depth-tables',((0x88ad9c,32),)),
                        ('ninja-texture-returns',((0x389d458,16),)),
                        ('ninja-queue-consumer-constants',((0x7dcb0c,4),(0x7dce50,4))),
                        ('player-animation-constants',tuple((address,4) for address in (0x7dcb2c,0x7dcb10,0x7dcb7c))),
                        ('ninja-sprite-constants',((0x9279d8,20),)+tuple((address,4) for address in (0x7db2c4,0x7dce6c,0x7e9b40,0x7dcb2c,0x7dcb10))),
                        ('event-acceleration-constants',EVENT_ACCELERATION_CONSTANTS),
                        ('projection-angle-extra-constants',PROJECTION_ANGLE_CONSTANTS[2:]),
                        ('player-position-constants',PLAYER_POSITION_CONSTANTS),
                        ('player-contact-attribute-constants',((0x7dcc98,4),(0x7dd588,4),(0x7e0060,4),(0x7e007c,4),(0x7dd584,4),(0x43ccac,4),(0x43ccb9,4),(0x43ccc6,4),(0x43ccd2,4),(0x43ccda,4))),
                        ('player-contact-response-constants',((0x7dcb2c,4),(0x7dcb10,4),(0x7dd570,4),(0x7e0080,4),(0x7dd58c,4),(0x7dcb0c,4),(0x7e6c30,4),(0x7dccd0,8),(0x7dccc8,8),(0x7e0b40,8))),
                        ('player-auxiliary-effect-table',((0x9275a0,0x438),)),
                        ('sonic-player-constants',tuple((address,4) for address in (0x7dcb2c,0x7dfff0,0x7dd554,0x7e0090,0x7dd57c,0x7df1bc,0x7e01a8,0x7dcb94,0x7e0844,0x7df1c8,0x7e00f4,0x7dcb34,0x7dd550))),
                        ('sonic-action-dispatch',((0x49bfbc,352),)),
                        ('player-scene-position-constants',tuple((address,4) for address in (0x7dcb10,0x7dcb2c,0x7dcc74,0x7dcc98,0x7dccd8,0x7dd540,0x7dd550,0x7dd570,0x7dd578,0x7dd584,0x7dd58c,0x7dfff0,0x7e0084,0x7e0088,0x7e008c,0x7e00e4,0x7e0118,0x7e0128,0x7e012c,0x7e0174,0x7e0178,0x7e017c,0x7e0180,0x7e0184,0x7e0188))+tuple((address,8) for address in (0x7dccc8,0x7dccd0,0x7dcce8))),
                        ('player-reaction-constants',((0x7dcb2c,4),(0x7dcb10,4),(0x7e6c30,4),(0x7e0b40,8),*PROJECTION_ANGLE_CONSTANTS[2:])),
                        ('player-mesh-query-constants',((0x7dcc98,4),(0x7dccd8,4),(0x7dcb0c,4),(0x453c18,4),(0x7dccd0,8),(0x7e05f8,8))),
                        ('floor-angle-constants',((0x7e66d8,4),(0x7e66d4,4),(0x7e00e0,4),(0x7dccd0,8),(0x7dccc8,8),(0x7dcce8,8))),
                        ('crt-floor-constants',((0x840d90,80),)),
                        ('player-scene-constants',((0x7df1c8,4),(0x7e0600,4),(0x7e0028,4))),
                        ('player-path-constants',((0x7dcb2c,4),(0x7dcb10,4),(0x7dd57c,4),(0x840d90,80))),
                        ('player-direction-reset-constants',((0x7dcb2c,4),(0x7dcb10,4),(0x7dd57c,4),(0x840d90,80),*PROJECTION_ANGLE_CONSTANTS[2:],(0x7e0b40,8))),
                        ('player-contact-initialization-constants',tuple((address,4) for address in (
                            0x7dcb2c,0x7dd584,0x7dd58c,0x7dd540,0x7e0078,0x7e0074,
                            0x7e0070,0x7e006c,0x7e0068,0x7e0064,0x7dff54,0x7e0060,0x7dcc98))),
                        ('coordinate-axis-constants',((0x7dcb2c,4),(0x7dcb10,4)))):
        packet=b''.join(image.read(address,size) for address,size in ranges)
        (args.output / (name+'.bin')).write_bytes(packet)
        report[name]=dict(bytes=len(packet),sha256=hashlib.sha256(packet).hexdigest(),
            fields=[dict(address=hex(address),size=size,file_offset=image.file_offset(address,size),
                         bytes=image.read(address,size).hex()) for address,size in ranges])
    (args.output / 'source.json').write_text(json.dumps(report,indent=2)+'\n')
    (args.output / 'emerald-coast.mesh').write_bytes(b'SDXMESH1' + struct.pack('<I', len(mesh) // 32) + mesh)
    print(json.dumps(dict(source_sha256=image.sha256, col_count=stage['col_count'],
                           triangle_count=stage['triangle_count'], mesh_types=stage['mesh_types'],
                           output=str(args.output.resolve())), indent=2))


if __name__ == '__main__':
    main()
