#ifndef _B_NUMBER_FORMAT_PARAMETERS_H_
#define _B_NUMBER_FORMAT_PARAMETERS_H_

#include <SupportDefs.h>

enum number_format_sign_policy {
	B_USE_NEGATIVE_SIGN_ONLY,
	B_USE_SPACE_FOR_POSITIVE_SIGN,
	B_USE_POSITIVE_SIGN,
};

enum number_format_base {
	B_FLEXIBLE_DECIMAL_BASE	= 0,	// same as B_DECIMAL_BASE when formatting,
									// but recognizes octal and hexadecimal
									// numbers by prefix when parsing
	B_OCTAL_BASE			= 8,
	B_DECIMAL_BASE			= 10,
	B_HEXADECIMAL_BASE		= 16,
};

class BNumberFormatParameters {
	public:
		BNumberFormatParameters(const BNumberFormatParameters *parent = NULL);
		BNumberFormatParameters(const BNumberFormatParameters &other);
		~BNumberFormatParameters();

		void SetUseGrouping(bool useGrouping);
		bool UseGrouping() const;

		void SetSignPolicy(number_format_sign_policy policy);
		number_format_sign_policy SignPolicy() const;

		void SetBase(number_format_base base);
		number_format_base Base() const;

		void SetUseBasePrefix(bool useBasePrefix);
		bool UseBasePrefix() const;

		void SetMinimalIntegerDigits(size_t minIntegerDigits);
		size_t MinimalIntegerDigits() const;

		void SetParentNumberParameters(const BNumberFormatParameters *parent);
		const BNumberFormatParameters *ParentNumberParameters() const;

		BNumberFormatParameters &operator=(
			const BNumberFormatParameters &other);

	private:
		const BNumberFormatParameters	*fParent;
		bool							fUseGrouping;
		number_format_sign_policy		fSignPolicy;
		number_format_base				fBase;
		bool							fUseBasePrefix;
		size_t							fMinimalIntegerDigits;
		uint32							fFlags;
};

#endif	// _B_NUMBER_FORMAT_PARAMETERS_H_