// Retained initialized data with no recovered code references. The original
// filenames and symbol names are unknown; this grouping and these names are
// provisional. These are typed values, not storage added to fill address gaps.

// Original VA 00440B00. Nibble permutation: (5 * index + 11) & 15.
// It precedes the CIC response tables; its original use is not established.
unsigned char LegacyNibblePermutation[16] = {
    11, 0, 5, 10, 15, 4, 9, 14, 3, 8, 13, 2, 7, 12, 1, 6
};

// Original VA 00477754. Bit length of a nonzero high nibble, including its
// four-bit position. Zero has the separately encoded result zero.
unsigned char LegacyHighNibbleBitLength[16] = {
    0, 5, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8
};

// Original VA 00477764. Bit length of the nibble index; original use unknown.
unsigned char LegacyLowNibbleBitLength[16] = {
    0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4
};

extern void DynarecNoOperation();
typedef void (__cdecl *LegacyCallback)();

// Original VAs 00477774..00477797 contain nine pointers to the shared empty
// function at 0041FEF0. Their original callback signatures and whether they
// were one array or separate declarations cannot be recovered from references.
LegacyCallback LegacyEmptyCallbacks[9] = {
    DynarecNoOperation, DynarecNoOperation, DynarecNoOperation,
    DynarecNoOperation, DynarecNoOperation, DynarecNoOperation,
    DynarecNoOperation, DynarecNoOperation, DynarecNoOperation
};

// Original VA 00477798. This value equals the NTSC clock rate used by the
// memory and CPU control routines; no reference to this retained object survives.
unsigned int LegacyNtscClockRate = 48681812;

// Original VA 004777A0. Four-by-four single-precision identity matrix.
// Original owner and use are unknown. No object is inferred for 0047779C.
float LegacyIdentityMatrix[4][4] = {
    {1.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 1.0f}
};
