#version 330 core

in vec2 textureCoordinate;

out vec4 finalColor;

uniform sampler2D sceneTexture;
uniform vec2 trailStart;
uniform vec2 trailEnd;
uniform float animationTime;
uniform float distortionStrength;

void main()
{
    vec2 trailVector = trailEnd - trailStart;
    float trailLength = length(trailVector);
    vec2 direction = vec2(1.0, 0.0);
    if (trailLength > 0.0001) {
        direction = trailVector / trailLength;
    }

    //Measure position along the trail and sideways from the trail.
    vec2 fromTrailStart = textureCoordinate - trailStart;
    float distanceAlongTrail = dot(fromTrailStart, direction);
    vec2 sidewaysDirection = vec2(-direction.y, direction.x);
    float distanceFromTrail = abs(dot(fromTrailStart, sidewaysDirection));

    //Keep the effect short and thin, so the sphere itself stays unchanged.
    float alongTrail = smoothstep(-0.004, 0.012, distanceAlongTrail) *
        (1.0 - smoothstep(
            trailLength - 0.012,
            trailLength + 0.004,
            distanceAlongTrail
        ));
    float thinBand = 1.0 - smoothstep(
        0.004,
        0.011,
        distanceFromTrail
    );

    //A travelling wave makes only the background beside the trail bend.
    float wave = sin(distanceAlongTrail * 180.0 - animationTime * 8.0);
    vec2 bentCoordinate = textureCoordinate + sidewaysDirection * wave *
        thinBand * alongTrail * distortionStrength * 0.012;

    finalColor = texture(sceneTexture, bentCoordinate);
}
