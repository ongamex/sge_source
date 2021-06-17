#include "OptionPermutator.h"
#include "sge_utils/sge_utils.h"

namespace sge {

void OptionPermuataor::build(const std::vector<OptionDesc>& options) {
	*this = OptionPermuataor();

	allOptions = options;

	numAllPermutations = 1;
	for (const OptionDesc& option : allOptions) {
		numAllPermutations *= int(option.possibleValues.size());
	}

	allPermutations.resize(numAllPermutations);

	for (int iPerm = 0; iPerm < numAllPermutations; ++iPerm) {
		std::vector<int>& permutation = allPermutations[iPerm];
		permutation.resize(allOptions.size());

		int period = 1;
		for (int iOpt = 0; iOpt < allOptions.size(); ++iOpt) {
			const OptionDesc& option = allOptions[iOpt];
			permutation[option.optionId] = (iPerm / period) % option.possibleValues.size();
			period *= int(option.possibleValues.size());
		}
	}
}

int OptionPermuataor::computePermutationIndex(const OptionChoice* const optionChoices, const int numOptions) const {
	if (numAllPermutations == 0 || numOptions != allOptions.size()) {
		sgeAssert(false && "All options must be specified!");
		return -1;
	}

	int resultPermIdx = 0;

	int period = 1;
	for (int iOpt = 0; iOpt < numOptions; ++iOpt) {
		if (optionChoices[iOpt].optionId != allOptions[iOpt].optionId) {
			sgeAssert(false && "Options aren't ordered correctly!");
			return -1;
		}

		const OptionDesc& option = allOptions[iOpt];
		const int optionIdx = optionChoices[iOpt].optionChoice;

		if (optionIdx < 0 || optionIdx >= allOptions[iOpt].possibleValues.size()) {
			sgeAssert(false && "Option index isn't in range!");
			return -1;
		}

		resultPermIdx += period * optionIdx;
		period *= int(option.possibleValues.size());
	}

	sgeAssert(resultPermIdx < numAllPermutations);

	//#ifdef SGE_USE_DEBUG
	//	for(int t = 0; t < numOptions; ++t) {
	//		sgeAssert(allPermutations[resultPermIdx][t] == optionChoices[t].optionChoice);
	//	}
	//#endif

	return resultPermIdx;
}

} // namespace sge