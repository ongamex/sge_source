#include "GraphicsCommon_d3d11.h"

namespace sge {

D3D11_VIEWPORT Viewport_D3D11_Native(const Rect2s& viewport) {
	D3D11_VIEWPORT result;

	result.Width = (float)viewport.width;
	result.Height = (float)viewport.height;
	result.TopLeftX = (float)viewport.x;
	result.TopLeftY = (float)viewport.y;
	result.MinDepth = 0.f;
	result.MaxDepth = 1.f;

	return result;
}

UINT ResourceBindFlags_D3D11_Native(const unsigned bindFlags) {
	UINT retval = 0;
	retval |= (bindFlags & ResourceBindFlags::VertexBuffer) ? D3D11_BIND_VERTEX_BUFFER : 0;
	retval |= (bindFlags & ResourceBindFlags::IndexBuffer) ? D3D11_BIND_INDEX_BUFFER : 0;
	retval |= (bindFlags & ResourceBindFlags::ConstantBuffer) ? D3D11_BIND_CONSTANT_BUFFER : 0;
	retval |= (bindFlags & ResourceBindFlags::ShaderResource) ? D3D11_BIND_SHADER_RESOURCE : 0;
	retval |= (bindFlags & ResourceBindFlags::StreamOutput) ? D3D11_BIND_STREAM_OUTPUT : 0;

	return retval;
}

void ResourceUsage_D3D11_Native(ResourceUsage::Enum type, D3D11_USAGE& usage, UINT& cpuAcessFlags) {
	switch (type) {
		case ResourceUsage::Immutable:
			usage = D3D11_USAGE_IMMUTABLE;
			cpuAcessFlags = 0;
			return;

		case ResourceUsage::Default:
			usage = D3D11_USAGE_DEFAULT;
			cpuAcessFlags = 0;
			return;

		case ResourceUsage::Dynamic:
			usage = D3D11_USAGE_DYNAMIC;
			cpuAcessFlags = D3D11_CPU_ACCESS_WRITE;
			return;
	}

	sgeAssert(false); // Unimplemented type.
}

D3D11_MAP Map_D3D11_Native(const Map::Enum map) {
	switch (map) {
		case Map::Read:
			return D3D11_MAP_READ;
		case Map::Write:
			return D3D11_MAP_WRITE;
		case Map::ReadWrite:
			return D3D11_MAP_READ_WRITE;
		case Map::WriteDiscard:
			return D3D11_MAP_WRITE_DISCARD;
	}

	// Unimplemented type.
	sgeAssert(false);
	return D3D11_MAP_READ;
}

void TextureFormat_D3D11_Native(const TextureFormat::Enum format, DXGI_FORMAT& srv, DXGI_FORMAT& dsv, DXGI_FORMAT& typeless) {
	switch (format) {
		case TextureFormat::Unknown:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_UNKNOWN;
			srv = DXGI_FORMAT_UNKNOWN;
			return;

		case TextureFormat::R32G32B32A32_FLOAT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R32G32B32A32_TYPELESS;
			srv = DXGI_FORMAT_R32G32B32A32_FLOAT;
			return;
		case TextureFormat::R32G32B32A32_UINT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R32G32B32A32_TYPELESS;
			srv = DXGI_FORMAT_R32G32B32A32_UINT;
			return;
		case TextureFormat::R32G32B32A32_SINT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R32G32B32A32_TYPELESS;
			srv = DXGI_FORMAT_R32G32B32A32_SINT;
			return;
		case TextureFormat::R32G32B32_FLOAT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R32G32B32_TYPELESS;
			srv = DXGI_FORMAT_R32G32B32_FLOAT;
			return;
		case TextureFormat::R32G32B32_UINT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R32G32B32_TYPELESS;
			srv = DXGI_FORMAT_R32G32B32_UINT;
			return;
		case TextureFormat::R32G32B32_SINT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R32G32B32_TYPELESS;
			srv = DXGI_FORMAT_R32G32B32_SINT;
			return;
		case TextureFormat::R16G16B16A16_FLOAT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R16G16B16A16_TYPELESS;
			srv = DXGI_FORMAT_R16G16B16A16_FLOAT;
			return;
		case TextureFormat::R16G16B16A16_UNORM:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R16G16B16A16_TYPELESS;
			srv = DXGI_FORMAT_R16G16B16A16_UNORM;
			return;
		case TextureFormat::R16G16B16A16_UINT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R16G16B16A16_TYPELESS;
			srv = DXGI_FORMAT_R16G16B16A16_UINT;
			return;
		case TextureFormat::R16G16B16A16_SNORM:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R16G16B16A16_TYPELESS;
			srv = DXGI_FORMAT_R16G16B16A16_SNORM;
			return;
		case TextureFormat::R16G16B16A16_SINT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R16G16B16A16_TYPELESS;
			srv = DXGI_FORMAT_R16G16B16A16_SINT;
			return;
		case TextureFormat::R32G32_FLOAT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R32G32_TYPELESS;
			srv = DXGI_FORMAT_R32G32_FLOAT;
			return;
		case TextureFormat::R32G32_UINT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R32G32_TYPELESS;
			srv = DXGI_FORMAT_R32G32_UINT;
			return;
		case TextureFormat::R32G32_SINT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R32G32_TYPELESS;
			srv = DXGI_FORMAT_R32G32_SINT;
			return;

		case TextureFormat::R10G10B10A2_UNORM:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R10G10B10A2_TYPELESS;
			srv = DXGI_FORMAT_R10G10B10A2_UNORM;
			return;
		case TextureFormat::R10G10B10A2_UINT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R10G10B10A2_TYPELESS;
			srv = DXGI_FORMAT_R10G10B10A2_UINT;
			return;
		case TextureFormat::R11G11B10_FLOAT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_UNKNOWN;
			srv = DXGI_FORMAT_R11G11B10_FLOAT;
			return;
		case TextureFormat::R8G8B8A8_UNORM:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R8G8B8A8_TYPELESS;
			srv = DXGI_FORMAT_R8G8B8A8_UNORM;
			return;
		case TextureFormat::R8G8B8A8_UNORM_SRGB:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R8G8B8A8_TYPELESS;
			srv = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			return;
		case TextureFormat::R8G8B8A8_UINT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R8G8B8A8_TYPELESS;
			srv = DXGI_FORMAT_R8G8B8A8_UINT;
			return;
		case TextureFormat::R8G8B8A8_SNORM:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R8G8B8A8_TYPELESS;
			srv = DXGI_FORMAT_R8G8B8A8_SNORM;
			return;
		case TextureFormat::R8G8B8A8_SINT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_UNKNOWN;
			srv = DXGI_FORMAT_R8G8B8A8_SINT;
			return;
		case TextureFormat::R16G16_FLOAT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R16G16_TYPELESS;
			srv = DXGI_FORMAT_R16G16_FLOAT;
			return;
		case TextureFormat::R16G16_UNORM:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R16G16_TYPELESS;
			srv = DXGI_FORMAT_R16G16_UNORM;
			return;
		case TextureFormat::R16G16_UINT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R16G16_TYPELESS;
			srv = DXGI_FORMAT_R16G16_UINT;
			return;
		case TextureFormat::R16G16_SNORM:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R16G16_TYPELESS;
			srv = DXGI_FORMAT_R16G16_SNORM;
			return;
		case TextureFormat::R16G16_SINT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R16G16_TYPELESS;
			srv = DXGI_FORMAT_R16G16_SINT;
			return;

		case TextureFormat::R32_FLOAT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R32_TYPELESS;
			srv = DXGI_FORMAT_R32_FLOAT;
			return;
		case TextureFormat::R32_UINT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R32_TYPELESS;
			srv = DXGI_FORMAT_R32_UINT;
			return;
		case TextureFormat::R32_SINT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R32_TYPELESS;
			srv = DXGI_FORMAT_R32_SINT;
			return;

		case TextureFormat::R8G8_UNORM:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R8G8_TYPELESS;
			srv = DXGI_FORMAT_R8G8_UNORM;
			return;
		case TextureFormat::R8G8_UINT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R8G8_TYPELESS;
			srv = DXGI_FORMAT_R8G8_UINT;
			return;
		case TextureFormat::R8G8_SNORM:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R8G8_TYPELESS;
			srv = DXGI_FORMAT_R8G8_SNORM;
			return;
		case TextureFormat::R8G8_SINT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R8G8_TYPELESS;
			srv = DXGI_FORMAT_R8G8_SINT;
			return;
		case TextureFormat::R16_FLOAT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R16_TYPELESS;
			srv = DXGI_FORMAT_R16_FLOAT;
			return;

		case TextureFormat::R16_UNORM:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R16_TYPELESS;
			srv = DXGI_FORMAT_R16_UNORM;
			return;
		case TextureFormat::R16_UINT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R16_TYPELESS;
			srv = DXGI_FORMAT_R16_UINT;
			return;
		case TextureFormat::R16_SNORM:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R16_TYPELESS;
			srv = DXGI_FORMAT_R16_SNORM;
			return;
		case TextureFormat::R16_SINT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R16_TYPELESS;
			srv = DXGI_FORMAT_R16_SINT;
			return;
		case TextureFormat::R8_UNORM:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R8_TYPELESS;
			srv = DXGI_FORMAT_R8_UNORM;
			return;
		case TextureFormat::R8_UINT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R8_TYPELESS;
			srv = DXGI_FORMAT_R8_UINT;
			return;
		case TextureFormat::R8_SNORM:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R8_TYPELESS;
			srv = DXGI_FORMAT_R8_SNORM;
			return;
		case TextureFormat::R8_SINT:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R8_TYPELESS;
			srv = DXGI_FORMAT_R8_SINT;
			return;

		case TextureFormat::A8_UNORM:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_R8_TYPELESS;
			srv = DXGI_FORMAT_A8_UNORM;
			return;

		case TextureFormat::BC1_UNORM:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_BC1_TYPELESS;
			srv = DXGI_FORMAT_BC1_UNORM;
			return;
		case TextureFormat::BC2_UNORM:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_BC2_TYPELESS;
			srv = DXGI_FORMAT_BC2_UNORM;
			return;
		case TextureFormat::BC3_UNORM:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_BC3_TYPELESS;
			srv = DXGI_FORMAT_BC3_UNORM;
			return;

		case TextureFormat::BC4_UNORM:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_BC4_TYPELESS;
			srv = DXGI_FORMAT_BC4_UNORM;
			return;
		case TextureFormat::BC4_SNORM:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_BC4_TYPELESS;
			srv = DXGI_FORMAT_BC4_SNORM;
			return;

		case TextureFormat::BC5_UNORM:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_BC5_TYPELESS;
			srv = DXGI_FORMAT_BC5_UNORM;
			return;
		case TextureFormat::BC5_SNORM:
			dsv = DXGI_FORMAT_UNKNOWN;
			typeless = DXGI_FORMAT_BC5_TYPELESS;
			srv = DXGI_FORMAT_BC5_SNORM;
			return;


		case TextureFormat::D24_UNORM_S8_UINT:
			dsv = DXGI_FORMAT_D24_UNORM_S8_UINT;
			typeless = DXGI_FORMAT_R24G8_TYPELESS;
			srv = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			return;
		case TextureFormat::D32_FLOAT:
			dsv = DXGI_FORMAT_D32_FLOAT;
			typeless = DXGI_FORMAT_R32_TYPELESS;
	}

	// Unimplemented format.
	sgeAssert(false);
}

D3D11_SUBRESOURCE_DATA TextureData_D3D11_Native(const TextureData& texData) {
	D3D11_SUBRESOURCE_DATA result;

	result.pSysMem = texData.data;
	result.SysMemPitch = (UINT)texData.rowByteSize;
	result.SysMemSlicePitch = (UINT)texData.sliceByteSize;

	return result;
}

D3D11_TEXTURE_ADDRESS_MODE TextureAddressMode_D3D11_Native(const TextureAddressMode::Enum& mode) {
	if (mode == TextureAddressMode::Repeat)
		return D3D11_TEXTURE_ADDRESS_WRAP;
	if (mode == TextureAddressMode::ClampEdge)
		return D3D11_TEXTURE_ADDRESS_CLAMP;
	if (mode == TextureAddressMode::ClampBorder)
		return D3D11_TEXTURE_ADDRESS_BORDER;

	sgeAssert(false); // unknown address mode
	return D3D11_TEXTURE_ADDRESS_WRAP;
}

D3D11_FILTER TextureFilter_D3D11_Native(const TextureFilter::Enum& filter) {
	if (filter == TextureFilter::Min_Mag_Mip_Point)
		return D3D11_FILTER_MIN_MAG_MIP_POINT;
	if (filter == TextureFilter::Min_Mag_Mip_Linear)
		return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	if (filter == TextureFilter::Anisotropic)
		return D3D11_FILTER_ANISOTROPIC;

	sgeAssert(false);
	return D3D11_FILTER_MIN_MAG_MIP_POINT;
}

D3D11_SAMPLER_DESC SamplerDesc_D3D11_Native(SamplerDesc const& desc) {
	D3D11_SAMPLER_DESC retval;

	retval.Filter = TextureFilter_D3D11_Native(desc.filter);
	retval.AddressU = TextureAddressMode_D3D11_Native(desc.addressModes[0]);
	retval.AddressV = TextureAddressMode_D3D11_Native(desc.addressModes[1]);
	retval.AddressW = TextureAddressMode_D3D11_Native(desc.addressModes[2]);
	retval.BorderColor[0] = desc.colorBorder[0];
	retval.BorderColor[1] = desc.colorBorder[1];
	retval.BorderColor[2] = desc.colorBorder[2];
	retval.BorderColor[3] = desc.colorBorder[3];
	retval.ComparisonFunc = D3D11_COMPARISON_NEVER; // [TODO]
	retval.MaxAnisotropy = desc.maxAnisotropy;
	retval.MinLOD = desc.minLOD; // [TODO]
	retval.MaxLOD = desc.maxLOD; // [TODO]
	retval.MipLODBias = 0.f;     // [TODO]

	return retval;
}

DXGI_FORMAT UniformType_GetDX_DXGI_FORMAT(const UniformType::Enum uniformType) {
	switch (uniformType) {
		case UniformType::Unknown:
			return DXGI_FORMAT_UNKNOWN;

		case UniformType::Float:
			return DXGI_FORMAT_R32_FLOAT;
		case UniformType::Float2:
			return DXGI_FORMAT_R32G32_FLOAT;
		case UniformType::Float3:
			return DXGI_FORMAT_R32G32B32_FLOAT;
		case UniformType::Float4:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;

		case UniformType::Double:
		case UniformType::Double2:
		case UniformType::Double3:
		case UniformType::Double4:
			sgeAssert(false);
			return DXGI_FORMAT_UNKNOWN;

		case UniformType::Int:
			return DXGI_FORMAT_R32_SINT;
		case UniformType::Int2:
			return DXGI_FORMAT_R32G32_SINT;
		case UniformType::Int3:
			return DXGI_FORMAT_R32G32B32_SINT;
		case UniformType::Int4:
			return DXGI_FORMAT_R32G32B32A32_SINT;

		case UniformType::Uint16:
			return DXGI_FORMAT_R16_UINT;

		case UniformType::Uint:
			return DXGI_FORMAT_R32_UINT;
		case UniformType::Uint2:
			return DXGI_FORMAT_R32G32_UINT;
		case UniformType::Uint3:
			return DXGI_FORMAT_R32G32B32_UINT;
		case UniformType::Uint4:
			return DXGI_FORMAT_R32G32B32A32_UINT;

		case UniformType::Float3x3:
		case UniformType::Float4x4:
			sgeAssert(false);
			return DXGI_FORMAT_UNKNOWN;

		case UniformType::Int_RGBA_Unorm_IA:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
	};

	sgeAssert(false);
	return DXGI_FORMAT_UNKNOWN;
}

D3D11_BUFFER_DESC BufferDesc_D3D11_Native(const BufferDesc& desc) {
	D3D11_BUFFER_DESC retval = {0};

	ResourceUsage_D3D11_Native(desc.usage, retval.Usage, retval.CPUAccessFlags);
	retval.ByteWidth = (UINT)desc.sizeBytes;
	retval.BindFlags = ResourceBindFlags_D3D11_Native(desc.bindFlags);
	retval.StructureByteStride = (UINT)desc.structByteStride;
	retval.MiscFlags = 0;

	return retval;
}

D3D11_FILL_MODE FillMode_D3D11_Native(const FillMode::Enum& fillMode) {
	if (fillMode == FillMode::Solid)
		return D3D11_FILL_SOLID;
	else if (fillMode == FillMode::Wireframe)
		return D3D11_FILL_WIREFRAME;
	else
		sgeAssert(false);

	return D3D11_FILL_WIREFRAME;
}

D3D11_CULL_MODE CullMode_D3D11_Native(const CullMode::Enum& cullMode) {
	switch (cullMode) {
		case CullMode::Back:
			return D3D11_CULL_BACK;
		case CullMode::Front:
			return D3D11_CULL_FRONT;
		case CullMode::None:
			return D3D11_CULL_NONE;
	}

	sgeAssert(false);
	return D3D11_CULL_NONE;
}

D3D11_COMPARISON_FUNC DepthComparisonFunc_D3D11_Native(const DepthComparisonFunc::Enum& comaprisonFunc) {
	switch (comaprisonFunc) {
		case DepthComparisonFunc::Never:
			return D3D11_COMPARISON_NEVER;
		case DepthComparisonFunc::Always:
			return D3D11_COMPARISON_ALWAYS;
		case DepthComparisonFunc::Less:
			return D3D11_COMPARISON_LESS;
		case DepthComparisonFunc::LessEqual:
			return D3D11_COMPARISON_LESS_EQUAL;
		case DepthComparisonFunc::Greater:
			return D3D11_COMPARISON_GREATER;
		case DepthComparisonFunc::GreaterEqual:
			return D3D11_COMPARISON_GREATER_EQUAL;
		case DepthComparisonFunc::Equal:
			return D3D11_COMPARISON_EQUAL;
		case DepthComparisonFunc::NotEqual:
			return D3D11_COMPARISON_NOT_EQUAL;
	}

	sgeAssert(false);
	return D3D11_COMPARISON_ALWAYS;
}

D3D_PRIMITIVE_TOPOLOGY PrimitiveTopology_D3D11_Native(const PrimitiveTopology::Enum& topology) {
	switch (topology) {
		case PrimitiveTopology::TriangleList:
			return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			break;
		case PrimitiveTopology::TriangleStrip:
			return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			break;
		case PrimitiveTopology::LineList:
			return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
			break;
		case PrimitiveTopology::LineStrip:
			return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
			break;
		case PrimitiveTopology::PointList:
			return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
			break;
	}

	return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

D3D11_RASTERIZER_DESC RasterDesc_GetD3D11Nativce(const RasterDesc& desc) {
	D3D11_RASTERIZER_DESC result;

	result.FillMode = FillMode_D3D11_Native(desc.fillMode);
	result.CullMode = CullMode_D3D11_Native(desc.cullMode);
	result.FrontCounterClockwise = !desc.backFaceCCW;
	result.DepthBias = int(desc.depthBiasAdd);
	result.DepthBiasClamp = 0.f;
	result.SlopeScaledDepthBias = desc.depthBiasSlope;
	result.DepthClipEnable = TRUE;
	result.ScissorEnable = desc.useScissor;
	result.MultisampleEnable = FALSE;
	result.AntialiasedLineEnable = FALSE;

	return result;
}

D3D11_DEPTH_STENCIL_DESC DepthStencilDesc_D3D11_Native(const DepthStencilDesc& desc) {
	const D3D11_DEPTH_STENCILOP_DESC defaultDepthStencilOp = {D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP,
	                                                          D3D11_COMPARISON_ALWAYS};

	D3D11_DEPTH_STENCIL_DESC result;

	result.DepthEnable = (BOOL)desc.depthTestEnabled;
	result.DepthWriteMask = (desc.depthWriteEnabled) ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	result.DepthFunc = DepthComparisonFunc_D3D11_Native(desc.comparisonFunc);
	result.StencilEnable = FALSE;
	result.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	result.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	result.FrontFace = defaultDepthStencilOp;
	result.BackFace = defaultDepthStencilOp;

	return result;
}

D3D11_BLEND Blend_D3D11_Native(Blend::Enum blend) {
	if (blend == Blend::Zero)
		return D3D11_BLEND_ZERO;
	else if (blend == Blend::One)
		return D3D11_BLEND_ONE;

	else if (blend == Blend::SrcColor)
		return D3D11_BLEND_SRC_COLOR;
	else if (blend == Blend::InvSrcColor)
		return D3D11_BLEND_INV_SRC_COLOR;

	else if (blend == Blend::DestColor)
		return D3D11_BLEND_DEST_COLOR;
	else if (blend == Blend::InvDestColor)
		return D3D11_BLEND_INV_DEST_COLOR;

	// Alpha only.
	else if (blend == Blend::Alpha_Src)
		return D3D11_BLEND_SRC_ALPHA;
	else if (blend == Blend::Alpha_InvSrc)
		return D3D11_BLEND_INV_SRC_ALPHA;
	else if (blend == Blend::Alpha_Dest)
		return D3D11_BLEND_DEST_ALPHA;
	else if (blend == Blend::Alpha_InvDest)
		return D3D11_BLEND_INV_DEST_ALPHA;

	// Unknown BlendType
	sgeAssert(false);
	return D3D11_BLEND_ONE;
}

D3D11_BLEND_OP BlendOp_D3D11_Native(BlendOp::Enum blendOp) {
	if (blendOp == BlendOp::Add)
		return D3D11_BLEND_OP_ADD;
	if (blendOp == BlendOp::Sub)
		return D3D11_BLEND_OP_SUBTRACT;
	if (blendOp == BlendOp::RevSub)
		return D3D11_BLEND_OP_REV_SUBTRACT;
	if (blendOp == BlendOp::Min)
		return D3D11_BLEND_OP_MIN;
	if (blendOp == BlendOp::Max)
		return D3D11_BLEND_OP_MAX;

	// Unknown BlendType
	sgeAssert(false);
	return D3D11_BLEND_OP_ADD;
}

D3D11_RENDER_TARGET_BLEND_DESC BlendDesc_D3D11_Native(const BlendDesc& desc) {
	D3D11_RENDER_TARGET_BLEND_DESC blendDesc;

	blendDesc.BlendEnable = desc.enabled;
	blendDesc.SrcBlend = Blend_D3D11_Native(desc.srcBlend);
	blendDesc.DestBlend = Blend_D3D11_Native(desc.destBlend);
	blendDesc.BlendOp = BlendOp_D3D11_Native(desc.blendOp);
	blendDesc.SrcBlendAlpha = Blend_D3D11_Native(desc.alphaSrcBlend);
	blendDesc.DestBlendAlpha = Blend_D3D11_Native(desc.alphaDestBlend);
	blendDesc.BlendOpAlpha = BlendOp_D3D11_Native(desc.alphaBlendOp);
	blendDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	return blendDesc;
}

D3D11_BLEND_DESC BlendStateDesc_D3D11_Native(const BlendStateDesc& desc) {
	sgeAssert(SGE_ARRSZ(D3D11_BLEND_DESC::RenderTarget) == desc.blendDesc.size());

	D3D11_BLEND_DESC result;
	result.AlphaToCoverageEnable = false;
	result.IndependentBlendEnable = desc.independentBlend;

	const size_t numRenderTargets = (std::min)(SGE_ARRSZ(result.RenderTarget), desc.blendDesc.size());

	for (int t = 0; t < numRenderTargets; ++t) {
		result.RenderTarget[t] = BlendDesc_D3D11_Native(desc.blendDesc[t]);
	}

	return result;
}

D3D11_QUERY QueryType_D3D11_Native(QueryType::Enum const queryType) {
	switch (queryType) {
		case QueryType::NumSamplesPassedDepthStencilTest:
			return D3D11_QUERY_OCCLUSION;
		case QueryType::AnySamplePassedDepthStencilTest:
			return D3D11_QUERY_OCCLUSION_PREDICATE;
	}

	// Should never happen
	sgeAssert(false);
	return (D3D11_QUERY)(D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM3 + 1); // Try to return something invalid.
}

D3D11_TEXTURECUBE_FACE signedAxis_toTexCubeFaceIdx_D3D11(const SignedAxis sa) {
	switch (sa) {
		case axis_x_pos:
			return D3D11_TEXTURECUBE_FACE_POSITIVE_X;
		case axis_x_neg:
			return D3D11_TEXTURECUBE_FACE_NEGATIVE_X;
		case axis_y_pos:
			return D3D11_TEXTURECUBE_FACE_POSITIVE_Y;
		case axis_y_neg:
			return D3D11_TEXTURECUBE_FACE_NEGATIVE_Y;
		case axis_z_pos:
			return D3D11_TEXTURECUBE_FACE_POSITIVE_Z;
		case axis_z_neg:
			return D3D11_TEXTURECUBE_FACE_NEGATIVE_Z;
	}

	sgeAssert(false);
	return D3D11_TEXTURECUBE_FACE_POSITIVE_X;
}

} // namespace sge
