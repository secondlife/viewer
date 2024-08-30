#pragma once

// ============================================================================
// RlvActions - developer-friendly non-RLVa code facing class, use in lieu of RlvHandler whenever possible
//

class RlvActions
{
    // ================
    // Helper functions
    // ================
public:
    /*
     * Convenience function to check if RLVa is enabled without having to include rlvhandler.h
     */
    static bool isRlvEnabled();
};

// ============================================================================
