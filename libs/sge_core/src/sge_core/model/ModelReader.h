#pragma once

#include "Model.h"
#include "sge_core/sgecore_api.h"
#include "sge_utils/utils/IStream.h"

namespace sge {

namespace Model {
	class SGE_CORE_API ModelReader {
		struct DataChunkDesc {
			int chunkId = 0;
			size_t byteOffset = 0;
			size_t sizeBytes = 0;
		};

	  public:
		static PrimitiveTopology::Enum PrimitiveTolologyFromString(const char* str);
		static UniformType::Enum UniformTypeFromString(const char* str);

		ModelReader() = default;
		~ModelReader() {}

		bool Load(const LoadSettings loadSets, IReadStream* const irs, Model& model);

	  private:
		IReadStream* irs;
		std::vector<DataChunkDesc> dataChunksDesc;

		const DataChunkDesc& FindDataChunkDesc(const int chunkId) const;

		// CAUTION: These functions assume that irs points at the BEGINING of data chunks.
		template <typename T>
		void LoadDataChunk(std::vector<T>& resultBuffer, const int chunkId);
		void LoadDataChunkRaw(void* const ptr, const size_t ptrExpectedSize, const int chunkId);
	};

} // namespace Model

} // namespace sge
