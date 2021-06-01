#ifndef SGE_LIB_PBR
#define SGE_LIB_PBR

//--------------------------------------------------------------------
// GGX
// https://learnopengl.com/PBR/Lighting
//--------------------------------------------------------------------
float DistributionGGX(float3 N, float3 H, float a) {
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float k) {
	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float k) {
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx1 = GeometrySchlickGGX(NdotV, k);
	float ggx2 = GeometrySchlickGGX(NdotL, k);

	return ggx1 * ggx2;
}

float3 fresnelSchlick(float cosTheta, float3 F0) {
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness) {
	return F0 + (max(float3(1.f - roughness, 1.f - roughness, 1.f - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

// ----------------------------------------------------------------------------
// GGX Importance sample
// ----------------------------------------------------------------------------
float VanDerCorpus(int n, int  base) {
	float invBase = 1.0 / float(base);
	float denom = 1.0;
	float result = 0.0;

	for (int  i = 0; i < 32; ++i) {
		if (n > 0) {
			denom = fmod(float(n), 2.0);
			result += denom * invBase;
			invBase = invBase / 2.0;
			n = int (float(n) / 2.0);
		}
	}

	return result;
}

float2 HammersleyNoBitOps(int  i, int  N) {
	return float2(float(i) / float(N), VanDerCorpus(i, 2));
}

float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness) {
	float a = roughness * roughness;

	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	// from spherical coordinates to cartesian coordinates
	float3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

	// from tangent-space vector to world-space sample vector
	float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	float3 tangent = normalize(cross(up, N));
	float3 bitangent = cross(N, tangent);

	float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

#endif // SGE_LIB_PBR