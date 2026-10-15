/* **********************************************************************************
 */
/*                                                                                    */
/*                                                      :::::::::: :::::::::
 * :::::: */
/*   RenderingPrograms.cpp                             :+:        :+:    :+: :+:
 * : */
/*                                                    +:+        +:+    +:+ +:+
 */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+
 */
/*                                                  +#+        +#+    +#+ +#+ */
/*   Created: 2026/10/08          by flaviobc      #+#        #+# #+#    #+# #+#
 */
/*   Updated: 2026/10/08          by flaviobc     ###        ######### ########
 */
/*                                                                                    */
/* **********************************************************************************
 */

#include "graphics/resources/RenderingPrograms.hpp"

#include "graphics/runtime/OpenGlFunctions.hpp"

#include <cstdio>

namespace
{
static constexpr const char *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texCoord;
layout (location = 4) in uvec4 jointIndices;
layout (location = 5) in vec4 jointWeights;
layout (location = 6) in vec4 instancePositionScale;
layout (location = 7) in vec2 instanceRotation;
uniform mat4 modelViewProjection;
uniform mat4 worldViewProjection;
uniform mat4 modelMatrix;
uniform mat4 lightSpaceMatrix;
uniform mat3 normalMatrix;
uniform bool useSkinning;
uniform bool useInstancing;
uniform vec4 instanceColorization;
uniform vec4 emissiveColor;
uniform mat4 jointPalette[2];
out vec3 vertexColor;
out vec3 vertexNormal;
out vec3 worldPosition;
out vec4 lightSpacePosition;
out vec2 vertexTexCoord;
flat out vec4 vertexInstanceColorization;
flat out vec4 vertexInstanceEmission;
void main()
{
    vec4 localPosition = vec4(position, 1.0);
    vec3 localNormal = normal;
    if (useSkinning)
    {
        mat4 skinMatrix = jointWeights.x * jointPalette[int(jointIndices.x)] +
            jointWeights.y * jointPalette[int(jointIndices.y)] +
            jointWeights.z * jointPalette[int(jointIndices.z)] +
            jointWeights.w * jointPalette[int(jointIndices.w)];
        localPosition = skinMatrix * localPosition;
        localNormal = mat3(skinMatrix) * localNormal;
    }
    mat4 activeModelMatrix = modelMatrix;
    mat3 activeNormalMatrix = normalMatrix;
    if (useInstancing)
    {
        float cosine = instanceRotation.x;
        float sine = instanceRotation.y;
        float scale = instancePositionScale.w;
        activeModelMatrix = mat4(
            vec4(cosine * scale, 0.0, -sine * scale, 0.0),
            vec4(0.0, scale, 0.0, 0.0),
            vec4(sine * scale, 0.0, cosine * scale, 0.0),
            vec4(instancePositionScale.xyz, 1.0));
        activeNormalMatrix = mat3(
            vec3(cosine, 0.0, -sine), vec3(0.0, 1.0, 0.0),
            vec3(sine, 0.0, cosine));
    }
    vec4 activeWorldPosition = activeModelMatrix * localPosition;
    gl_Position = useInstancing
        ? worldViewProjection * activeWorldPosition
        : modelViewProjection * localPosition;
    vertexColor = color;
    vertexNormal = activeNormalMatrix * localNormal;
    worldPosition = activeWorldPosition.xyz;
    lightSpacePosition = lightSpaceMatrix * vec4(worldPosition, 1.0);
    vertexTexCoord = texCoord;
    vertexInstanceColorization = instanceColorization;
    vertexInstanceEmission = emissiveColor;
}
)";
static constexpr const char *fragmentShaderSource = R"(
#version 330 core
in vec3 vertexColor;
in vec3 vertexNormal;
in vec3 worldPosition;
in vec4 lightSpacePosition;
in vec2 vertexTexCoord;
flat in vec4 vertexInstanceColorization;
flat in vec4 vertexInstanceEmission;
uniform bool usePrimitiveColor;
uniform bool useBaseColorTexture;
uniform bool pointLightActive;
uniform vec4 primitiveColor;
uniform vec4 pointLightPositionIntensity;
uniform vec4 pointLightColorRadius;
uniform sampler2D baseColorTexture;
uniform sampler2D shadowMap;
layout (location = 0) out vec4 sceneColor;
layout (location = 1) out vec4 bloomColor;
float directionalShadow(vec3 normal, vec3 lightDirection)
{
    vec3 projected = lightSpacePosition.xyz / lightSpacePosition.w;
    projected = projected * 0.5 + 0.5;
    if (projected.z > 1.0 || projected.x < 0.0 || projected.x > 1.0 ||
        projected.y < 0.0 || projected.y > 1.0)
    {
        return 0.0;
    }
    float closestDepth = texture(shadowMap, projected.xy).r;
    float bias = max(0.0025 * (1.0 - dot(normal, lightDirection)), 0.0005);
    return projected.z - bias > closestDepth ? 1.0 : 0.0;
}
void main()
{
    vec4 baseColor = usePrimitiveColor ? primitiveColor : vec4(vertexColor, 1.0);
    if (useBaseColorTexture)
    {
        baseColor *= texture(baseColorTexture, vec2(vertexTexCoord.x, 1.0 - vertexTexCoord.y));
    }
    float materialLuminance = dot(baseColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    vec3 colorizedMaterial = materialLuminance * vertexInstanceColorization.rgb;
    baseColor.rgb = mix(baseColor.rgb, colorizedMaterial,
                        clamp(vertexInstanceColorization.a, 0.0, 1.0));
    vec3 normalizedNormal = normalize(vertexNormal);
    vec3 lightDirection = normalize(vec3(-0.45, 0.8, 0.35));
    float lambert = max(dot(normalizedNormal, lightDirection), 0.0);
    float shadow = usePrimitiveColor
        ? directionalShadow(normalizedNormal, lightDirection)
        : 0.0;
    float intensity = usePrimitiveColor
        ? 0.35 + 0.65 * lambert * (1.0 - shadow)
        : 1.0;
    vec3 toLight = pointLightPositionIntensity.xyz - worldPosition;
    float distanceToLight = length(toLight);
    vec3 pointLightDirection = distanceToLight > 0.0001
        ? toLight / distanceToLight
        : vec3(0.0, 0.0, 0.0);
    float pointLambert = max(dot(normalizedNormal, pointLightDirection), 0.0);
    float attenuation = pointLightActive
        ? max(1.0 - distanceToLight / max(pointLightColorRadius.a, 0.0001), 0.0)
        : 0.0;
    vec3 pointLight = usePrimitiveColor && pointLightActive
        ? pointLightColorRadius.rgb * pointLightPositionIntensity.a *
              attenuation * pointLambert
        : vec3(0.0, 0.0, 0.0);
    vec3 emission = usePrimitiveColor ? vertexInstanceEmission.rgb : vec3(0.0);
    vec3 color = baseColor.rgb * (intensity + pointLight) + emission;
    sceneColor = vec4(color, baseColor.a);
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    bloomColor = vec4(brightness > 1.0 ? color : vec3(0.0), 1.0);
}
)";
static constexpr const char *fullscreenVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texCoord;
out vec2 screenTexCoord;
void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
    screenTexCoord = texCoord;
}
)";
static constexpr const char *hudVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texCoord;
uniform mat4 hudTransform;
out vec2 iconTexCoord;
void main()
{
    gl_Position = hudTransform * vec4(position, 0.0, 1.0);
    iconTexCoord = texCoord;
}
)";
static constexpr const char *hudFragmentShaderSource = R"(
#version 330 core
in vec2 iconTexCoord;
uniform sampler2D iconAtlas;
uniform bool useIconAtlas;
uniform vec4 hudTint;
out vec4 outputColor;
void main()
{
    outputColor = useIconAtlas ? texture(iconAtlas, iconTexCoord) : hudTint;
}
)";
static constexpr const char *blurFragmentShaderSource = R"(
#version 330 core
in vec2 screenTexCoord;
uniform sampler2D bloomImage;
uniform bool horizontal;
out vec4 outputColor;
void main()
{
    vec2 texel = 1.0 / vec2(textureSize(bloomImage, 0));
    vec3 color = texture(bloomImage, screenTexCoord).rgb * 0.227027;
    for (int offset = 1; offset <= 4; ++offset)
    {
        float weight = offset == 1 ? 0.1945946 :
            (offset == 2 ? 0.1216216 : (offset == 3 ? 0.054054 : 0.016216));
        vec2 direction = horizontal ? vec2(texel.x * float(offset), 0.0) :
            vec2(0.0, texel.y * float(offset));
        color += texture(bloomImage, screenTexCoord + direction).rgb * weight;
        color += texture(bloomImage, screenTexCoord - direction).rgb * weight;
    }
    outputColor = vec4(color, 1.0);
}
)";
static constexpr const char *compositeFragmentShaderSource = R"(
#version 330 core
in vec2 screenTexCoord;
uniform sampler2D sceneImage;
uniform sampler2D bloomImage;
out vec4 outputColor;
void main()
{
    const float bloomStrength = 0.65;
    vec3 color = texture(sceneImage, screenTexCoord).rgb +
        bloomStrength * texture(bloomImage, screenTexCoord).rgb;
    outputColor = vec4(color, 1.0);
}
)";
static constexpr const char *shadowVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 position;
layout (location = 4) in uvec4 jointIndices;
layout (location = 5) in vec4 jointWeights;
layout (location = 6) in vec4 instancePositionScale;
layout (location = 7) in vec2 instanceRotation;
uniform mat4 shadowModelViewProjection;
uniform mat4 lightSpaceMatrix;
uniform bool useSkinning;
uniform bool useInstancing;
uniform mat4 jointPalette[2];
void main()
{
    vec4 localPosition = vec4(position, 1.0);
    if (useSkinning)
    {
        mat4 skinMatrix = jointWeights.x * jointPalette[int(jointIndices.x)] +
            jointWeights.y * jointPalette[int(jointIndices.y)] +
            jointWeights.z * jointPalette[int(jointIndices.z)] +
            jointWeights.w * jointPalette[int(jointIndices.w)];
        localPosition = skinMatrix * localPosition;
    }
    if (useInstancing)
    {
        float cosine = instanceRotation.x;
        float sine = instanceRotation.y;
        float scale = instancePositionScale.w;
        mat4 instanceModelMatrix = mat4(
            vec4(cosine * scale, 0.0, -sine * scale, 0.0),
            vec4(0.0, scale, 0.0, 0.0),
            vec4(sine * scale, 0.0, cosine * scale, 0.0),
            vec4(instancePositionScale.xyz, 1.0));
        gl_Position = lightSpaceMatrix * instanceModelMatrix * localPosition;
    }
    else
    {
        gl_Position = shadowModelViewProjection * localPosition;
    }
}
)";
static constexpr const char *shadowFragmentShaderSource = R"(
#version 330 core
void main()
{
}
)";

constexpr const char *spriteVertexShaderSource = R"(#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 color;
uniform mat4 worldViewProjection;
out vec2 spriteUv;
out vec4 spriteColor;
void main()
{
    gl_Position = worldViewProjection * vec4(position, 1.0);
    spriteUv = uv;
    spriteColor = color;
}
)";

constexpr const char *spriteFragmentShaderSource = R"(#version 330 core
in vec2 spriteUv;
in vec4 spriteColor;
uniform sampler2D spriteImage;
uniform float bloomIntensity;
uniform vec4 colorMultiplier;
layout(location = 0) out vec4 sceneColor;
layout(location = 1) out vec4 bloomColor;
void main()
{
    vec4 mask = texture(spriteImage, spriteUv);
    vec4 shadedSprite = vec4(mask.rgb * spriteColor.rgb * colorMultiplier.rgb,
        mask.a * spriteColor.a * colorMultiplier.a);
    sceneColor = shadedSprite;
    bloomColor = vec4(shadedSprite.rgb * bloomIntensity, shadedSprite.a);
}
)";

GLuint compileShader(engine::graphics::detail::OpenGlFunctions &gl, GLenum type,
                     const char *source, const char *name)
{
	const GLuint shader = gl.createShader(type);
	if (shader == 0)
	{
		std::fprintf(stderr, "%s shader creation failed.\n", name);
		return 0;
	}
	gl.shaderSource(shader, 1, &source, nullptr);
	gl.compileShader(shader);
	GLint succeeded = GL_FALSE;
	gl.getShaderiv(shader, GL_COMPILE_STATUS, &succeeded);
	if (succeeded == GL_TRUE)
	{
		return shader;
	}
	GLchar infoLog[1024]{};
	gl.getShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
	std::fprintf(stderr, "%s shader compilation failed:\n%s\n", name, infoLog);
	gl.deleteShader(shader);
	return 0;
}

GLuint createScreenProgram(engine::graphics::detail::OpenGlFunctions &gl,
                           const char *vertexSource, const char *fragmentSource,
                           const char *name)
{
	const GLuint vertexShader =
	    compileShader(gl, GL_VERTEX_SHADER, vertexSource, "Screen vertex");
	const GLuint fragmentShader =
	    compileShader(gl, GL_FRAGMENT_SHADER, fragmentSource, name);
	if (vertexShader == 0 || fragmentShader == 0)
	{
		if (vertexShader != 0)
		{
			gl.deleteShader(vertexShader);
		}
		if (fragmentShader != 0)
		{
			gl.deleteShader(fragmentShader);
		}
		return 0;
	}
	const GLuint program = gl.createProgram();
	if (program == 0)
	{
		gl.deleteShader(vertexShader);
		gl.deleteShader(fragmentShader);
		return 0;
	}
	gl.attachShader(program, vertexShader);
	gl.attachShader(program, fragmentShader);
	gl.linkProgram(program);
	gl.deleteShader(vertexShader);
	gl.deleteShader(fragmentShader);
	GLint linked = GL_FALSE;
	gl.getProgramiv(program, GL_LINK_STATUS, &linked);
	if (linked == GL_TRUE)
	{
		return program;
	}
	GLchar infoLog[1024]{};
	gl.getProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
	std::fprintf(stderr, "%s program link failed:\n%s\n", name, infoLog);
	gl.deleteProgram(program);
	return 0;
}
} // namespace

namespace engine::graphics::detail
{
bool RenderingPrograms::create(OpenGlFunctions &gl)
{
	const GLuint vertexShader =
	    compileShader(gl, GL_VERTEX_SHADER, vertexShaderSource, "Vertex");
	if (vertexShader == 0)
	{
		return false;
	}
	const GLuint fragmentShader =
	    compileShader(gl, GL_FRAGMENT_SHADER, fragmentShaderSource, "Fragment");
	if (fragmentShader == 0)
	{
		gl.deleteShader(vertexShader);
		return false;
	}

	main_.handle = gl.createProgram();
	if (main_.handle == 0)
	{
		gl.deleteShader(vertexShader);
		gl.deleteShader(fragmentShader);
		std::fputs("Shader program creation failed.\n", stderr);
		return false;
	}
	gl.attachShader(main_.handle, vertexShader);
	gl.attachShader(main_.handle, fragmentShader);
	gl.linkProgram(main_.handle);
	gl.deleteShader(vertexShader);
	gl.deleteShader(fragmentShader);
	GLint linked = GL_FALSE;
	gl.getProgramiv(main_.handle, GL_LINK_STATUS, &linked);
	if (linked != GL_TRUE)
	{
		GLchar infoLog[1024]{};
		gl.getProgramInfoLog(main_.handle, sizeof(infoLog), nullptr, infoLog);
		std::fprintf(stderr, "Shader program link failed:\n%s\n", infoLog);
		return false;
	}

	const GLuint shadowVertexShader = compileShader(
	    gl, GL_VERTEX_SHADER, shadowVertexShaderSource, "Shadow vertex");
	if (shadowVertexShader == 0)
	{
		return false;
	}
	const GLuint shadowFragmentShader = compileShader(
	    gl, GL_FRAGMENT_SHADER, shadowFragmentShaderSource, "Shadow fragment");
	if (shadowFragmentShader == 0)
	{
		gl.deleteShader(shadowVertexShader);
		return false;
	}
	shadow_.handle = gl.createProgram();
	if (shadow_.handle == 0)
	{
		gl.deleteShader(shadowVertexShader);
		gl.deleteShader(shadowFragmentShader);
		std::fputs("Shadow shader program creation failed.\n", stderr);
		return false;
	}
	gl.attachShader(shadow_.handle, shadowVertexShader);
	gl.attachShader(shadow_.handle, shadowFragmentShader);
	gl.linkProgram(shadow_.handle);
	gl.deleteShader(shadowVertexShader);
	gl.deleteShader(shadowFragmentShader);
	gl.getProgramiv(shadow_.handle, GL_LINK_STATUS, &linked);
	if (linked != GL_TRUE)
	{
		GLchar infoLog[1024]{};
		gl.getProgramInfoLog(shadow_.handle, sizeof(infoLog), nullptr, infoLog);
		std::fprintf(stderr, "Shadow shader program link failed:\n%s\n",
		             infoLog);
		return false;
	}

	blur_.handle = createScreenProgram(gl, fullscreenVertexShaderSource,
	                                   blurFragmentShaderSource, "Bloom blur");
	composite_.handle =
	    createScreenProgram(gl, fullscreenVertexShaderSource,
		                    compositeFragmentShaderSource, "HDR composite");
	hud_.handle = createScreenProgram(gl, hudVertexShaderSource,
	                                  hudFragmentShaderSource, "HUD icon");
	sprite_.handle = createScreenProgram(
	    gl, spriteVertexShaderSource, spriteFragmentShaderSource, "VFX sprite");
	if (blur_.handle == 0 || composite_.handle == 0 || hud_.handle == 0 ||
	    sprite_.handle == 0)
	{
		return false;
	}

	main_.uniforms.modelViewProjection =
	    gl.getUniformLocation(main_.handle, "modelViewProjection");
	main_.uniforms.worldViewProjection =
	    gl.getUniformLocation(main_.handle, "worldViewProjection");
	main_.uniforms.lightSpaceMatrix =
	    gl.getUniformLocation(main_.handle, "lightSpaceMatrix");
	main_.uniforms.shadowMap = gl.getUniformLocation(main_.handle, "shadowMap");
	main_.uniforms.normalMatrix =
	    gl.getUniformLocation(main_.handle, "normalMatrix");
	main_.uniforms.modelMatrix =
	    gl.getUniformLocation(main_.handle, "modelMatrix");
	main_.uniforms.pointLightActive =
	    gl.getUniformLocation(main_.handle, "pointLightActive");
	main_.uniforms.pointLightPositionIntensity =
	    gl.getUniformLocation(main_.handle, "pointLightPositionIntensity");
	main_.uniforms.pointLightColorRadius =
	    gl.getUniformLocation(main_.handle, "pointLightColorRadius");
	main_.uniforms.usePrimitiveColor =
	    gl.getUniformLocation(main_.handle, "usePrimitiveColor");
	main_.uniforms.primitiveColor =
	    gl.getUniformLocation(main_.handle, "primitiveColor");
	main_.uniforms.instanceColorization =
	    gl.getUniformLocation(main_.handle, "instanceColorization");
	main_.uniforms.emissiveColor =
	    gl.getUniformLocation(main_.handle, "emissiveColor");
	main_.uniforms.useBaseColorTexture =
	    gl.getUniformLocation(main_.handle, "useBaseColorTexture");
	main_.uniforms.baseColorTexture =
	    gl.getUniformLocation(main_.handle, "baseColorTexture");
	main_.uniforms.useSkinning =
	    gl.getUniformLocation(main_.handle, "useSkinning");
	main_.uniforms.useInstancing =
	    gl.getUniformLocation(main_.handle, "useInstancing");
	main_.uniforms.jointPalette =
	    gl.getUniformLocation(main_.handle, "jointPalette[0]");

	shadow_.uniforms.modelViewProjection =
	    gl.getUniformLocation(shadow_.handle, "shadowModelViewProjection");
	shadow_.uniforms.lightSpaceMatrix =
	    gl.getUniformLocation(shadow_.handle, "lightSpaceMatrix");
	shadow_.uniforms.useSkinning =
	    gl.getUniformLocation(shadow_.handle, "useSkinning");
	shadow_.uniforms.useInstancing =
	    gl.getUniformLocation(shadow_.handle, "useInstancing");
	shadow_.uniforms.jointPalette =
	    gl.getUniformLocation(shadow_.handle, "jointPalette[0]");

	blur_.uniforms.image = gl.getUniformLocation(blur_.handle, "bloomImage");
	blur_.uniforms.horizontal =
	    gl.getUniformLocation(blur_.handle, "horizontal");

	composite_.uniforms.scene =
	    gl.getUniformLocation(composite_.handle, "sceneImage");
	composite_.uniforms.bloom =
	    gl.getUniformLocation(composite_.handle, "bloomImage");

	hud_.uniforms.atlas = gl.getUniformLocation(hud_.handle, "iconAtlas");
	hud_.uniforms.transform =
	    gl.getUniformLocation(hud_.handle, "hudTransform");
	hud_.uniforms.useAtlas = gl.getUniformLocation(hud_.handle, "useIconAtlas");
	hud_.uniforms.tint = gl.getUniformLocation(hud_.handle, "hudTint");

	sprite_.uniforms.image =
	    gl.getUniformLocation(sprite_.handle, "spriteImage");
	sprite_.uniforms.worldViewProjection =
	    gl.getUniformLocation(sprite_.handle, "worldViewProjection");
	sprite_.uniforms.bloomIntensity =
	    gl.getUniformLocation(sprite_.handle, "bloomIntensity");
	sprite_.uniforms.colorMultiplier =
	    gl.getUniformLocation(sprite_.handle, "colorMultiplier");
	if (sprite_.uniforms.image < 0 || sprite_.uniforms.bloomIntensity < 0 ||
	    sprite_.uniforms.colorMultiplier < 0 ||
	    sprite_.uniforms.worldViewProjection < 0 ||
	    main_.uniforms.modelViewProjection < 0 ||
	    main_.uniforms.worldViewProjection < 0 ||
	    main_.uniforms.lightSpaceMatrix < 0 || main_.uniforms.shadowMap < 0 ||
	    main_.uniforms.normalMatrix < 0 || main_.uniforms.modelMatrix < 0 ||
	    main_.uniforms.pointLightActive < 0 ||
	    main_.uniforms.pointLightPositionIntensity < 0 ||
	    main_.uniforms.pointLightColorRadius < 0 ||
	    main_.uniforms.usePrimitiveColor < 0 ||
	    main_.uniforms.primitiveColor < 0 ||
	    main_.uniforms.instanceColorization < 0 ||
	    main_.uniforms.emissiveColor < 0 ||
	    main_.uniforms.useBaseColorTexture < 0 ||
	    main_.uniforms.baseColorTexture < 0 || main_.uniforms.useSkinning < 0 ||
	    main_.uniforms.useInstancing < 0 || main_.uniforms.jointPalette < 0 ||
	    shadow_.uniforms.modelViewProjection < 0 ||
	    shadow_.uniforms.lightSpaceMatrix < 0 ||
	    shadow_.uniforms.useSkinning < 0 ||
	    shadow_.uniforms.useInstancing < 0 ||
	    shadow_.uniforms.jointPalette < 0 || blur_.uniforms.image < 0 ||
	    blur_.uniforms.horizontal < 0 || composite_.uniforms.scene < 0 ||
	    composite_.uniforms.bloom < 0 || hud_.uniforms.atlas < 0 ||
	    hud_.uniforms.transform < 0 || hud_.uniforms.useAtlas < 0 ||
	    hud_.uniforms.tint < 0)
	{
		std::fputs("Renderer shader uniform was not found.\n", stderr);
		return false;
	}

	gl.useProgram(main_.handle);
	gl.uniform1i(main_.uniforms.shadowMap, 0);
	gl.uniform1i(main_.uniforms.baseColorTexture, 1);
	gl.useProgram(blur_.handle);
	gl.uniform1i(blur_.uniforms.image, 0);
	gl.useProgram(composite_.handle);
	gl.uniform1i(composite_.uniforms.scene, 0);
	gl.uniform1i(composite_.uniforms.bloom, 1);
	gl.useProgram(sprite_.handle);
	gl.uniform1i(sprite_.uniforms.image, 0);
	gl.useProgram(hud_.handle);
	gl.uniform1i(hud_.uniforms.atlas, 0);
	return true;
}

void RenderingPrograms::destroy(OpenGlFunctions &gl)
{
	const auto deleteProgram = [&gl](GLuint &program)
	{
		if (program != 0)
		{
			gl.deleteProgram(program);
			program = 0;
		}
	};
	deleteProgram(main_.handle);
	deleteProgram(shadow_.handle);
	deleteProgram(blur_.handle);
	deleteProgram(composite_.handle);
	deleteProgram(hud_.handle);
	deleteProgram(sprite_.handle);
	sprite_.uniforms = {};
	main_.uniforms = {};
	shadow_.uniforms = {};
	blur_.uniforms = {};
	composite_.uniforms = {};
	hud_.uniforms = {};
}

const RenderingPrograms::Main &RenderingPrograms::main() const
{
	return main_;
}

const RenderingPrograms::Shadow &RenderingPrograms::shadow() const
{
	return shadow_;
}

const RenderingPrograms::Blur &RenderingPrograms::blur() const
{
	return blur_;
}

const RenderingPrograms::Composite &RenderingPrograms::composite() const
{
	return composite_;
}

const RenderingPrograms::Sprite &RenderingPrograms::sprite() const
{
	return sprite_;
}

const RenderingPrograms::Hud &RenderingPrograms::hud() const
{
	return hud_;
}
} // namespace engine::graphics::detail
