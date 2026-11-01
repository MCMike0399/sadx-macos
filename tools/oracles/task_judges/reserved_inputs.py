"""Additive task cohorts in the unchanged original judges' supported domains.

Importing this module generates nothing. The caller supplies an admitted seed;
Root's reserved seeds must not be used during generator development.
"""
import random


def arena_sequences(seed, sequences, steps):
    """Use owned allocation indices and free each allocation at most once."""
    if sequences < 1 or steps < 1:
        raise ValueError('Positive cohort bounds required')
    rng = random.Random(seed)
    result = []
    sizes = (1, 16, 31, 32, 33, 56, 64, 80, 96, 512, 4096, 0x34000)
    for _ in range(sequences):
        operations, live = [], []
        allocation = 0
        for _ in range(steps):
            if live and rng.randrange(3) == 0:
                selected = rng.choice(live)
                live.remove(selected)
                operations.append((1, selected))
            else:
                operations.append((0, rng.choice(sizes)))
                live.append(allocation)
                allocation += 1
        rng.shuffle(live)
        operations.extend((1, index) for index in live)
        result.append(operations)
    return result


def lifecycle_sequences(seed, sequences, steps):
    """Keep direct destruction candidates top-level; children follow their owner.

    Every creation, including a child, consumes an allocation index. This is the
    unchanged lifecycle fixture's bookkeeping, not a new graph or cleanup model.
    """
    if sequences < 1 or steps < 1:
        raise ValueError('Positive cohort bounds required')
    rng = random.Random(seed)
    result = []
    for _ in range(sequences):
        operations, top_level = [], []
        allocation = 0
        for _ in range(steps):
            if top_level and rng.randrange(4) == 0:
                selected = rng.choice(top_level)
                top_level.remove(selected)
                operations.append((3, selected, 0, 0))
            else:
                if top_level and rng.randrange(3) == 0:
                    parent = rng.choice(top_level)
                    operations.append((2, rng.randrange(16), parent, 0))
                else:
                    level = rng.choice((0, 1, 2, 3, 4, 5, 6, 8))
                    operations.append((1, rng.randrange(16), level, 0))
                    top_level.append(allocation)
                allocation += 1
        operations.extend((3, index, 0, 0) for index in top_level)
        result.append(operations)
    return result
