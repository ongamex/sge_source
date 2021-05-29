#pragma once

#include "sge_core/sgecore_api.h"
#include "sge_utils/utils/json.h"
#include <memory>

namespace sge {

namespace Model {
	struct Model;
	struct KeyFrames;
}

class SGE_CORE_API ModelWriter {
  public:
	struct DataChunk {
		DataChunk() = default;

		DataChunk(int id, const void* data, size_t sizeBytes)
		    : id(id)
		    , data(data)
		    , sizeBytes(sizeBytes) {}

		int id;
		const void* data;
		size_t sizeBytes;
	};

	ModelWriter() = default;
	~ModelWriter() {}

	bool write(const Model::Model& modelToWrite, IWriteStream* iws);
	bool write(const Model::Model& modelToWrite, const char* const filename);


  private:
	// Returns the chunk id.
	int NewChunkFromPtr(const void* const ptr, const size_t sizeBytes);
	char* newDataChunkWithSize(size_t sizeBytes, int& outChunkId);

	// This function assumes that the vector wont be resized(aka. the data pointer won't change).
	template <typename T>
	int NewChunkFromStdVector(const std::vector<T>& data) {
		return NewChunkFromPtr(data.data(), data.size() * sizeof(T));
	}

	// The actiual writer is implemented in those functions.
	void GenerateAnimations();
	void GenerateNodes();         // Add the "nodes" to the root.
	void GenerateMaterials();     // Add the "materials" to the root.
	void GenerateMeshesData();
	void GenerateCollisionData();

	/// @brief Generates the json and allocated a data chunks for the specified keyframes.
	JsonValue* generateKeyFrames(const Model::KeyFrames& keyfames);

	JsonValueBuffer jvb;
	std::vector<DataChunk> dataChunks; // A list of the data chunks that will end up written to the file.
	JsonValue* root;                   // The file header json element.
	const Model::Model* model;         // The working model

	/// Ultra ugly, we assume that these elements are going to get moved.
	/// [MODERL_WRITER_MOVE_ASSUME]
	std::vector<std::vector<char>> m_dynamicallyAlocatedPointersToDelete;
};

} // namespace sge
