#ifndef _B_GENERIC_NUMBER_FORMAT_H_ 
#define _B_GENERIC_NUMBER_FORMAT_H_ 

#include <FloatFormatParameters.h>
#include <IntegerFormatParameters.h>

struct format_field_position;

class BGenericNumberFormat {
	public:
		BGenericNumberFormat();
		~BGenericNumberFormat();

		status_t FormatInteger(const BIntegerFormatParameters *parameters,
							   int64 number, BString *buffer,
							   format_field_position *positions = NULL,
							   int32 positionCount = 1,
							   int32 *fieldCount = NULL,
							   bool allFieldPositions = false) const;

		status_t FormatInteger(const BIntegerFormatParameters *parameters,
							   int64 number, char *buffer, size_t bufferSize,
							   format_field_position *positions = NULL,
							   int32 positionCount = 1,
							   int32 *fieldCount = NULL,
							   bool allFieldPositions = false) const;

		// default number format parameters

		status_t SetDefaultIntegerFormatParameters(
			const BIntegerFormatParameters *parameters);
		BIntegerFormatParameters *DefaultIntegerFormatParameters();
		const BIntegerFormatParameters *DefaultIntegerFormatParameters() const;

		status_t SetDefaultFloatFormatParameters(
			const BFloatFormatParameters *parameters);
		BFloatFormatParameters *DefaultFloatFormatParameters();
		const BFloatFormatParameters *DefaultFloatFormatParameters() const;

		// other parameters configuring the formatter

		status_t SetDigitSymbols(const char **digits);
		status_t SetDecimalSeparator(const char *decimalSeparator);
		status_t SetGroupingInfo(const char **groupingSeparators,
			size_t separatorCount, size_t *groupSizes, size_t sizeCount);
		status_t SetExponentSymbol(const char *exponentSymbol,
			const char *upperCaseExponentSymbol = NULL);
		status_t SetSpecialNumberSymbols(const char *nan,
			const char *infinity, const char *negativeInfinity = NULL,
			const char *upperCaseNaN = NULL,
			const char *upperCaseInfinity = NULL,
			const char *upperCaseNegativeInfinity = NULL);
		status_t SetSignSymbols(const char *plusPrefix,
			const char *minusPrefix, const char *padPlusPrefix,
			const char *noForcePlusPrefix = NULL, const char *plusSuffix = NULL,
			const char *minusSuffix = NULL, const char *padPlusSuffix = NULL,
			const char *noForcePlusSuffix = NULL);
		status_t SetExponentSignSymbols(const char *plusPrefix,
			const char *minusPrefix, const char *plusSuffix = NULL,
			const char *minusSuffix = NULL);

	private:
		class Integer;
		class SignSymbols;
		class BufferWriter;
		class GroupingInfo;

		struct Symbol {
			char	*symbol;
			int		length;
			int		char_count;

			Symbol(const char *symbol = NULL);
			~Symbol();

			status_t SetTo(const char *symbol);
			void Unset()	{ SetTo(NULL); }
		};

		const Symbol *DigitSymbols() const;
		const Symbol *DecimalSeparator() const;
		const GroupingInfo *GetGroupingInfo() const;
		const Symbol *ExponentSymbol(bool upperCase = false) const;
		const Symbol *NaNSymbol(bool upperCase = false) const;
		const Symbol *InfinitySymbol(bool upperCase = false) const;
		const Symbol *NegativeInfinitySymbol(bool upperCase = false) const;
		const SignSymbols *GetSignSymbols() const;
		const SignSymbols *ExponentSignSymbols() const;

		static status_t _SetSymbol(Symbol **symbol, const char *str);

		BIntegerFormatParameters	fIntegerParameters;
		BFloatFormatParameters		fFloatParameters;
		Symbol						*fDigitSymbols;
		Symbol						*fDecimalSeparator;
		GroupingInfo				*fGroupingInfo;
		Symbol						*fExponentSymbol;
		Symbol						*fUpperCaseExponentSymbol;
		Symbol						*fNaNSymbol;
		Symbol						*fUpperCaseNaNSymbol;
		Symbol						*fInfinitySymbol;
		Symbol						*fUpperCaseInfinitySymbol;
		Symbol						*fNegativeInfinitySymbol;
		Symbol						*fUpperCaseNegativeInfinitySymbol;
		SignSymbols					*fSignSymbols;
		SignSymbols					*fExponentSignSymbols;
};

#endif	// _B_GENERIC_NUMBER_FORMAT_H_ 
