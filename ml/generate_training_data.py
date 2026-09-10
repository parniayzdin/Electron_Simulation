import math

n = 1
l = 0


def radial_density(radius):
    rho = 2.0 * radius / n

    wave = math.exp(-rho * 0.5)

    #Probability density = wave squared
    density = wave * wave

    return density


#Try a few different distances from the proton
for radius in [0.0, 0.5, 1.0, 1.5, 2.0, 3.0]:
    density = radial_density(radius)

    print("radius:", radius, "density:", density)