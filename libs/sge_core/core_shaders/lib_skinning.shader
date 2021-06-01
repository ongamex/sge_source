#ifndef SGE_LIB_SKINNING
#define SGE_LIB_SKINNING

float4x4 libSkining_getBoneTransform(int iBone, sampler2D bonesTransfTex) {
	const int2 boneTexSize = tex2Dsize(bonesTransfTex);
		
	const float mtxColumn_0_U = ((0.5 + 0.0) / 4.f);
	const float mtxColumn_1_U = ((0.5 + 1.0) / 4.f);
	const float mtxColumn_2_U = ((0.5 + 2.0) / 4.f);
	const float mtxColumn_3_U = ((0.5 + 3.0) / 4.f);	

	const float v = ((float)iBone + 0.5f) / (float)boneTexSize.y;

	const float4 c0 = tex2Dlod(bonesTransfTex, float4(mtxColumn_0_U, v, 0.f, 0.f));
	const float4 c1 = tex2Dlod(bonesTransfTex, float4(mtxColumn_1_U, v, 0.f, 0.f));
	const float4 c2 = tex2Dlod(bonesTransfTex, float4(mtxColumn_2_U, v, 0.f, 0.f));
	const float4 c3 = tex2Dlod(bonesTransfTex, float4(mtxColumn_3_U, v, 0.f, 0.f));

	// The matrix on CPU is column major, HLSL should be compiled with column major, but this is the syntax.
	const float4x4 mtx = float4x4(
		c0.x, c1.x, c2.x, c3.x,
		c0.y, c1.y, c2.y, c3.y,
		c0.z, c1.z, c2.z, c3.z,
		c0.w, c1.w, c2.w, c3.w);
		
	return mtx;
}

float4x4 libSkining_getSkinningTransform(int4 bonesIndices, float4 bonesWeights, sampler2D bonesTransfTex) {
	const float4x4 skinMtx = 
		  libSkining_getBoneTransform(bonesIndices.x, bonesTransfTex) * bonesWeights.x
		+ libSkining_getBoneTransform(bonesIndices.y, bonesTransfTex) * bonesWeights.y
		+ libSkining_getBoneTransform(bonesIndices.z, bonesTransfTex) * bonesWeights.z
		+ libSkining_getBoneTransform(bonesIndices.w, bonesTransfTex) * bonesWeights.w;
		
	return skinMtx;
}

#endif