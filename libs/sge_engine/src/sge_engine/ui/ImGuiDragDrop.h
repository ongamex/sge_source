#pragma once

#include "sge_core/SGEImGui.h"
#include "sge_engine/GameObject.h"
#include "sge_utils/utils/optional.h"
#include <optional>
#include <set>

namespace sge {

struct DragDropPayload {
	DragDropPayload() = default;
	virtual ~DragDropPayload() = default;
};

struct DragDropPayloadActor {
	static sge::Optional<std::set<sge::ObjectId>> decode(const ImGuiPayload* payload) {
		if (payload && payload->IsDataType("sgedd_actors") && payload->Data != nullptr && payload->DataSize % sizeof(int) == 0) {
			int numActors = payload->DataSize / sizeof(int);
			std::set<sge::ObjectId> objects;

			const int* idsRaw = (const int*)payload->Data;
			for (int t = 0; t < numActors; ++t) {
				objects.insert(sge::ObjectId(idsRaw[t]));
			}

			return objects;
		}
		return sge::NullOptional();
	}

	static sge::Optional<sge::ObjectId> decodeSingle(const ImGuiPayload* payload) {
		if (payload && payload->IsDataType("sgedd_actors") && payload->Data != nullptr && payload->DataSize == sizeof(int)) {
			const int* idRaw = (int*)payload->Data;
			return sge::ObjectId(*idRaw);
		}
		return sge::NullOptional();
	}

	static void setPayload(sge::ObjectId id) {
		int idInt = id.id;
		ImGui::SetDragDropPayload("sgedd_actors", &idInt, sizeof(idInt));
	}

	static void setPayload(vector_set<sge::ObjectId> ids) {
		std::vector<int> linearIds;
		linearIds.reserve(ids.size());
		for (const sge::ObjectId& id : ids) {
			linearIds.push_back(id.id);
		}

		ImGui::SetDragDropPayload("sgedd_actors", linearIds.data(), sizeof(linearIds[0]) * linearIds.size());
	}

	static sge::Optional<sge::ObjectId> acceptSingle() {
		if (ImGui::AcceptDragDropPayload("sgedd_actors")) {
			return decodeSingle(ImGui::GetDragDropPayload());
		}
		return sge::NullOptional();
	}

	static sge::Optional<std::set<sge::ObjectId>> accept() {
		if (ImGui::AcceptDragDropPayload("sgedd_actors")) {
			return decode(ImGui::GetDragDropPayload());
		}
		return sge::NullOptional();
	}
};

struct DragDropPayloadAsset {
	static sge::Optional<std::string> decode(const ImGuiPayload* payload) {
		if (payload && payload->IsDataType("sgedd_asset") && payload->Data != nullptr && payload->DataSize > 0) {
			return std::string((char*)payload->Data, payload->DataSize);
		}
		return sge::NullOptional();
	}

	static void setPayload(const std::string& assetPath) {
		if (assetPath.empty() == false) {
			ImGui::SetDragDropPayload("sgedd_asset", assetPath.c_str(), assetPath.size());
		}
	}

	static sge::Optional<std::string> accept() {
		if (ImGui::AcceptDragDropPayload("sgedd_asset")) {
			return decode(ImGui::GetDragDropPayload());
		}
		return sge::NullOptional();
	}
};

} // namespace sge
