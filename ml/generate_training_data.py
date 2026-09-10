"""Generate exact 1s labels in Bohr radii using the standard library."""
import csv
import math
from pathlib import Path


def radial_density(radius):
    """Unnormalized |R_10(r)|^2; the sampler adds the r^2 volume factor."""
    return math.exp(-2.0 * radius)


def samples(count=601, maximum=12.0):
    return [(maximum * i / (count - 1), radial_density(maximum * i / (count - 1)))
            for i in range(count)]


if __name__ == "__main__":
    destination = Path(__file__).with_name("training_data.csv")
    with destination.open("w", newline="") as output:
        writer = csv.writer(output)
        writer.writerow(["radius_bohr", "radial_density"])
        writer.writerows(samples())
    print(f"Wrote {destination}")
