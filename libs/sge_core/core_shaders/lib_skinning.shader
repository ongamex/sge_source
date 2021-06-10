#ifndef SGE_LIB_SKINNING
#define SGE_LIB_SKINNING

/// Samples a texture that represents bones transformation.
/// @param iBoneRow the row that contains the matrix for the specified bone.
/// @param bonesTransfTex the texture containing the matrices for the bones.
///        For more details on the texture see @libSkining_getSkinningTransform documentation.
/// @param is the height of the @bonesTransfTex casted to float.
float4x4 libSkining_getBoneTransform(int iBoneRow, sampler2D bonesTransfTex, float bonesTransfTexSizeYAsFloat) {
	const float mtxColumn_0_U = ((0.5 + 0.0) / 4.f);
	const float mtxColumn_1_U = ((0.5 + 1.0) / 4.f);
	const float mtxColumn_2_U = ((0.5 + 2.0) / 4.f);
	const float mtxColumn_3_U = ((0.5 + 3.0) / 4.f);	

	const float v = ((float)iBoneRow + 0.5f) / bonesTransfTexSizeYAsFloat;

	const float4 c0 = tex2Dlod(bonesTransfTex, float4(mtxColumn_0_U, v, 0.f, 0.f));
	const float4 c1 = tex2Dlod(bonesTransfTex, float4(mtxColumn_1_U, v, 0.f, 0.f));
	const float4 c2 = tex2Dlod(bonesTransfTex, float4(mtxColumn_2_U, v, 0.f, 0.f));
	const float4 c3 = tex2Dlod(bonesTransfTex, float4(mtxColumn_3_U, v, 0.f, 0.f));

	// Caution:
	// In HLSL the matrices are initialized row-by-row no matter how we use them.
	// The HLSLParser library (used to transpile HLSL-to-GLSL) respects that difference and still
	// initilizes them row-by-row, however the matrices are column major.
#ifndef OpenGL
	const float4x4 mtx = float4x4(
		c0.x, c1.x, c2.x, c3.x,
		c0.y, c1.y, c2.y, c3.y,
		c0.z, c1.z, c2.z, c3.z,
		c0.w, c1.w, c2.w, c3.w);
#else
	const float4x4 mtx = float4x4(
		c0.x, c0.y, c0.z, c0.w,
		c1.x, c1.y, c1.z, c1.w,
		c2.x, c2.y, c2.z, c2.w,
		c3.x, c3.y, c3.z, c3.w);
	
#endif

	return mtx;
}

/// Retrieves the bone transform form a texture representing bones transform matrices.
/// The texture @bonesTransfTex should with size 4 by number-of-bones RGBA.
/// Each row represents a single matrix. The 1st pixel in that row is the 1st column.
/// @param bonesIndices the indices of the bones that that vertex in that mesh.
///        As the @bonesTransfTex might hold the bones for more then one mesh (for example a 3D model with multiple meshes)
///        we need to be able to find where the 1st bone for the current mesh that is being rendered is.
///        This is specified by @firstBoneRowOffset.
/// @param firstBoneRowOffset is the offset in the texture, where the 1st bone for this mesh is.
/// @param bonesWeights is the weigths of the bones that influence the vertex.
/// @param bonesTransfTex the texture containing the matrices for the bones.
/// @result is the value matrix to be used to modify the vertex position in object space to get the desiered transformation.
float4x4 libSkining_getSkinningTransform(int4 bonesIndices, int firstBoneRowOffset, float4 bonesWeights, sampler2D bonesTransfTex) {
	const float fBonesTransfTexSizeY = (float)(tex2Dsize(bonesTransfTex).y);
	
	const int4 offsetedIndices = bonesIndices + firstBoneRowOffset;
	
	const float4x4 skinMtx = 
		  libSkining_getBoneTransform(offsetedIndices.x, bonesTransfTex, fBonesTransfTexSizeY) * bonesWeights.x
		+ libSkining_getBoneTransform(offsetedIndices.y, bonesTransfTex, fBonesTransfTexSizeY) * bonesWeights.y
		+ libSkining_getBoneTransform(offsetedIndices.z, bonesTransfTex, fBonesTransfTexSizeY) * bonesWeights.z
		+ libSkining_getBoneTransform(offsetedIndices.w, bonesTransfTex, fBonesTransfTexSizeY) * bonesWeights.w;
		
	return skinMtx;
}

#endif