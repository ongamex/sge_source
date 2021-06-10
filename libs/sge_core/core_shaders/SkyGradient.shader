uniform float4x4 uView;
uniform float4x4 uProj;
uniform float4x4 uProjViewInverse;
uniform float3 uCamPosWS;
uniform float3 uColorBottom;
uniform float3 uColorTop;

sampler2D uSkyTexture;

/// Generates UV coordinates for sampling spherical mapped textures from a direction.
/// Usually spherical mapped texture are used for hdri images form light probes.
/// @dir direction of sampling, must be normalized.
float2 directionToUV_spherical(float3 dir) {
    float2 uv = float2(atan2(dir.z, dir.x), asin(-dir.y));
    const float2 invAtan = float2(0.1591, 0.3183);
	uv *= invAtan;
    uv += float2(0.5, 0.5);
    return uv;
}

/// The implementation here is based on https://en.wikipedia.org/wiki/Cube_mapping
/// A function that gives a UV coordinates for sampling a cube-mapped 2D texture from a direction.
/// The texture that is itended to be sampled roughly looks like so:
///
///     *---*
///     |   |
/// *---*   *---*---*
/// |               |
/// *---*   *---*---*
///     |   |
///     *---*
///
/// @param dirNormalized normal is the direction to be used for generating the UVs. It needs to be normalized.
/// @param pixelSizeUV is optional (pass (0,0) if not used). It should be the size of a single pixel in UV space, 
///        it is used to avoid seems when sampling the texture, by modifying the UV a bit.
float2 directionToUV_cubeMapping(float3 dirNormalized, float2 pixelSizeUV) {
	const float x = dirNormalized.x;
	const float y = dirNormalized.y;
	const float z = dirNormalized.z;
		
	float absX = abs(x);
	float absY = abs(y);
	float absZ = abs(z);

	const bool isXPositive = x > 0.f;
	const bool isYPositive = y > 0.f;
	const bool isZPositive = z > 0.f;

	float maxAxis, uc, vc;
	int index; // The index of the axis that the direction hits on the cube.

	// POSITIVE X
	if (isXPositive && absX >= absY && absX >= absZ) {
		// u (0 to 1) goes from +z to -z
		// v (0 to 1) goes from -y to +y
		maxAxis = absX;
		uc = -z;
		vc = y;
		index = 0;
	}
	// NEGATIVE X
	if (!isXPositive && absX >= absY && absX >= absZ) {
		// u (0 to 1) goes from -z to +z
		// v (0 to 1) goes from -y to +y
		maxAxis = absX;
		uc = z;
		vc = y;
		index = 1;
	}
	// POSITIVE Y
	if (isYPositive && absY >= absX && absY >= absZ) {
		// u (0 to 1) goes from -x to +x
		// v (0 to 1) goes from +z to -z
		maxAxis = absY;
		uc = x;
		vc = -z;
		index = 2;
	}
	// NEGATIVE Y
	if (!isYPositive && absY >= absX && absY >= absZ) {
		// u (0 to 1) goes from -x to +x
		// v (0 to 1) goes from -z to +z
		maxAxis = absY;
		uc = x;
		vc = z;
		index = 3;
	}
	// POSITIVE Z
	if (isZPositive && absZ >= absX && absZ >= absY) {
		// u (0 to 1) goes from -x to +x
		// v (0 to 1) goes from -y to +y
		maxAxis = absZ;
		uc = x;
		vc = y;
		index = 4;
	}
	// NEGATIVE Z
	if (!isZPositive && absZ >= absX && absZ >= absY) {
		// u (0 to 1) goes from +x to -x
		// v (0 to 1) goes from -y to +y
		maxAxis = absZ;
		uc = -x;
		vc = y;
		index = 5;
	}

	// Convert range from -1 to 1 to 0 to 1
	float2 uv = float2(0.5f * (uc / maxAxis + 1.0f), 0.5f * (vc / maxAxis + 1.0f));

	// Move inwards the UVs near the edge of the face avoid having seems.
	// If we do not do that floating point precision + non-point sampling will
	// end up in nearing pixels that are not part of the cubemap to get used.
	if(uv.x < pixelSizeUV.x) {
		uv.x = pixelSizeUV.x;
	} else if(uv.x > 1.f - pixelSizeUV.x) {
		uv.x = 1.f - pixelSizeUV.x;
	}

	if(uv.y < pixelSizeUV.y) {
		uv.y = pixelSizeUV.y;
	} else if(uv.y > 1.f - pixelSizeUV.y) {
		uv.y = 1.f - pixelSizeUV.y;
	}

	// The UV coordinates so far were in OpenGL stlye, convert them to D3D style.
	uv.x = -uv.x + 1.f;
	uv.y = -uv.y + 1.f;
	uv = uv * float2(0.25f, 1.f / 3.f);

	// The function so far returned a face index and a UVs inside that face.
	// We need to return UVs that correspond to the big cube-mapped texture.
	if(index == 0) {
		// +X
		uv = uv + float2(0.f, 1.f / 3.f); 
	} else if(index == 1) {
		// -X
		uv = uv + float2(0.5f, 1.f / 3.f); 
	} else if(index == 2) {
		// +Y
		uv = uv + float2(0.25f, 0.f); 
	} else if(index == 3) {
		// -Y
		uv = uv + float2(0.25f, 2.f / 3.f); 
	} else if(index == 4) {
		// +Z
		uv = uv + float2(0.25f, 1.f / 3.f); 
	} else if(index == 5) {
		// -Z
		uv = uv + float2(0.75f, 1.f / 3.f); 
	}

	return uv;
}

//--------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------
struct VS_INPUT
{
	float3 a_position : a_position;
};

struct VS_OUTPUT {
	float4 SV_Position : SV_Position;
	float3 dirV : dirV;
};

VS_OUTPUT vsMain(VS_INPUT vsin)
{
	VS_OUTPUT res;
	float3 pointViewSpace = mul(uView, float4(vsin.a_position, 0.0)).xyz;
	float4 pointProj = mul(uProj, float4(pointViewSpace, 1.0));
	res.SV_Position = pointProj;
	res.dirV = vsin.a_position;

	return res;
}
//--------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------
struct PS_OUTPUT {
	float4 target0 : SV_Target0;
	float depth : SV_Depth;
};

PS_OUTPUT psMain(VS_OUTPUT IN)
{
	PS_OUTPUT psOut;
	
	const float4 posWS_h = mul(uProjViewInverse, IN.SV_Position);
    const float3 rayWs = normalize(posWS_h.xyz / posWS_h.w - uCamPosWS);
	const float2 uv = directionToUV_cubeMapping(normalize(IN.dirV), 4.f / float2(tex2Dsize(uSkyTexture)));
	const float3 color = tex2D(uSkyTexture, uv);
	
	psOut.target0 = float4(color, 1.f);
	psOut.depth = 1.0f;
	return psOut;
}