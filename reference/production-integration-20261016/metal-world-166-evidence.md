# R4 promotion and shared-renderer gate

Source 9e9570a. See metal-world-source-evidence.md for the contract, implementation
and preflight checks. The stable Station Square image at frame 2000 was inspected:
Sonic, pavement, stairs and the station building have geometry and textures.
The final driver passes the entire existing R4 log-prefix check with no skipped
frames; 443 commands, 439 textured, 75,008 colors. The earlier frame 1900 is still
under the game's fade and is not used as a stable geometry golden.

This judge change promotes world-3d from open to required, adds mode/stage assertions,
and adds make play-world. Existing thresholds, comparisons and earlier golden values
are retained. Only the new world-3d local golden entry is added, from the inspected
frame. Images stay ignored. The unchanged three guest stream hashes and performance
capture are retained in metal-world-166-summary.json. A missing texture projection
divisor is caught by the GPU fixture. The shared GPU test has no guest or asset inputs.

Stage performance is about 4.9 FPS offscreen over 100 frames, including boundary
readback. This is visible progress, not speedrun-practice readiness or Windows GPU
parity. Vertex fog, reflection-coordinate generation, active multi-stage textures,
unsupported formats/states and unvisited stages remain explicit unsupported scope.
