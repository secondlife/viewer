/**
 * @file v4color_glm_equivalence_test.cpp
 * @brief Differential tests for LLColor4 ↔ glm::vec4 (used as RGBA).
 *
 * **CRUCIAL CONTRAST WITH LLVector4:**
 * LLColor4 has *correctly behaving* arithmetic operators that touch
 * all four components (including alpha). It is NOT subject to the
 * "ignore W" disease that afflicts LLVector4. This test confirms
 * full 4-component equivalence with glm::vec4.
 *
 * It also pins down two clever quirks:
 *   - `operator*=(F32)` multiplies only RGB and leaves alpha alone
 *     (intended for tinting). Documented in the header comment as
 *     "no alpha change".
 *   - `operator%=(F32)` multiplies ONLY alpha and leaves RGB alone.
 *     Yes, the % operator is overloaded for "alpha channel scale".
 *     Cute, surprising, important to test.
 *
 * Implication for the GLM migration: anywhere the codebase uses
 * LLColor4 (instead of LLVector4) for color, mechanical replacement
 * with glm::vec4 is SAFE for `+`, `-`, full `+= LLColor4`, and the
 * binary `* LLColor4` for component-wise multiply. It is NOT safe
 * for `*= scalar` (LLColor4 leaves alpha alone, glm doesn't) or
 * `%= scalar` (no glm equivalent — would need a manual `c.a *= k`).
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "../test/lltut.h"

#include "../llmath.h"
#include "../v4color.h"
#include "../v3color.h"

#include <glm/glm.hpp>

#include <cmath>

namespace tut
{
    struct v4color_glm_equiv_data
    {
        static constexpr F32 kEps = 1e-5f;

        static glm::vec4 to_glm(const LLColor4& c)
        {
            return glm::vec4(c.mV[VRED], c.mV[VGREEN], c.mV[VBLUE], c.mV[VALPHA]);
        }

        static LLColor4 to_ll(const glm::vec4& c)
        {
            return LLColor4(c.x, c.y, c.z, c.w);
        }

        static bool color_near(const LLColor4& a, const glm::vec4& b, F32 eps = kEps)
        {
            return std::fabs(a.mV[VRED]   - b.x) <= eps
                && std::fabs(a.mV[VGREEN] - b.y) <= eps
                && std::fabs(a.mV[VBLUE]  - b.z) <= eps
                && std::fabs(a.mV[VALPHA] - b.w) <= eps;
        }
    };

    using v4color_glm_equiv_test = test_group<v4color_glm_equiv_data>;
    using v4color_glm_equiv_object = v4color_glm_equiv_test::object;
    tut::v4color_glm_equiv_test v4color_glm_equiv_testcase("v4color_glm_equivalence");

    // ---------- Construction ----------

    template<> template<>
    void v4color_glm_equiv_object::test<1>()
    {
        LLColor4 ll(0.5f, 0.7f, 0.3f, 0.8f);
        glm::vec4 gm(0.5f, 0.7f, 0.3f, 0.8f);
        ensure("rgba construction matches", color_near(ll, gm));
    }

    template<> template<>
    void v4color_glm_equiv_object::test<2>()
    {
        // 3-arg ctor in LLColor4 sets alpha to 1.0
        LLColor4 ll(0.5f, 0.7f, 0.3f);
        glm::vec4 gm(0.5f, 0.7f, 0.3f, 1.0f);
        ensure("3-arg ctor sets alpha=1", color_near(ll, gm));
    }

    template<> template<>
    void v4color_glm_equiv_object::test<3>()
    {
        // Default constructor in LLColor4 produces (0,0,0,1)
        LLColor4 ll;
        glm::vec4 gm(0.0f, 0.0f, 0.0f, 1.0f);
        ensure("default ctor is (0,0,0,1)", color_near(ll, gm));
    }

    // ---------- Addition / subtraction (FULL 4-component) ----------

    template<> template<>
    void v4color_glm_equiv_object::test<4>()
    {
        // Color addition adds all 4 components, including alpha. This
        // is the OPPOSITE of LLVector4's broken behavior — verify.
        LLColor4 a_ll(0.2f, 0.3f, 0.4f, 0.5f);
        LLColor4 b_ll(0.5f, 0.6f, 0.7f, 0.4f);
        LLColor4 sum_ll = a_ll + b_ll;

        glm::vec4 a_gm(0.2f, 0.3f, 0.4f, 0.5f);
        glm::vec4 b_gm(0.5f, 0.6f, 0.7f, 0.4f);
        glm::vec4 sum_gm = a_gm + b_gm;

        // Includes alpha — the result alpha should be 0.9, not 0.5
        ensure("color addition matches glm (incl alpha)",
               color_near(sum_ll, sum_gm));
        ensure_approximately_equals("alpha actually added",
                                    sum_ll.mV[VALPHA], 0.9f, 16);
    }

    template<> template<>
    void v4color_glm_equiv_object::test<5>()
    {
        LLColor4 a_ll(0.8f, 0.6f, 0.4f, 1.0f);
        LLColor4 b_ll(0.3f, 0.2f, 0.1f, 0.5f);
        LLColor4 diff_ll = a_ll - b_ll;

        glm::vec4 a_gm(0.8f, 0.6f, 0.4f, 1.0f);
        glm::vec4 b_gm(0.3f, 0.2f, 0.1f, 0.5f);
        glm::vec4 diff_gm = a_gm - b_gm;

        ensure("color subtraction matches (incl alpha)",
               color_near(diff_ll, diff_gm));
        ensure_approximately_equals("alpha actually subtracted",
                                    diff_ll.mV[VALPHA], 0.5f, 16);
    }

    // ---------- Scalar multiply (LL DIVERGENCE: leaves alpha alone) ----------

    template<> template<>
    void v4color_glm_equiv_object::test<6>()
    {
        // LLColor4::operator*=(F32) multiplies RGB only, leaves alpha
        // alone — intended for tinting. Document the divergence from
        // glm::vec4 * scalar (which scales all 4).
        LLColor4 ll(0.4f, 0.6f, 0.8f, 0.5f);
        ll *= 0.5f;  // dim by half

        // Expected: rgb halved, alpha unchanged
        ensure_approximately_equals("r dimmed", ll.mV[VRED],   0.2f, 16);
        ensure_approximately_equals("g dimmed", ll.mV[VGREEN], 0.3f, 16);
        ensure_approximately_equals("b dimmed", ll.mV[VBLUE],  0.4f, 16);
        ensure_approximately_equals("a unchanged by *=", ll.mV[VALPHA], 0.5f, 16);

        // glm DOES scale alpha
        glm::vec4 gm(0.4f, 0.6f, 0.8f, 0.5f);
        gm *= 0.5f;
        ensure_approximately_equals("glm a IS scaled", gm.w, 0.25f, 16);

        // So LL *= and glm *= DIVERGE on alpha
        ensure("LL *= and glm *= diverge on alpha",
               std::fabs(ll.mV[VALPHA] - gm.w) > 1e-4f);
    }

    // ---------- Alpha-only scale operator (no glm equivalent) ----------

    template<> template<>
    void v4color_glm_equiv_object::test<7>()
    {
        // LLColor4::operator%=(F32) is "scale alpha only, leave rgb alone".
        // Cute overload of % for alpha. No glm equivalent.
        LLColor4 ll(0.4f, 0.6f, 0.8f, 0.5f);
        ll %= 0.5f;  // halve alpha only

        ensure_approximately_equals("r unchanged by %=", ll.mV[VRED],   0.4f, 16);
        ensure_approximately_equals("g unchanged by %=", ll.mV[VGREEN], 0.6f, 16);
        ensure_approximately_equals("b unchanged by %=", ll.mV[VBLUE],  0.8f, 16);
        ensure_approximately_equals("a halved", ll.mV[VALPHA], 0.25f, 16);
    }

    // ---------- Component-wise color * color ----------

    template<> template<>
    void v4color_glm_equiv_object::test<8>()
    {
        // LLColor4::operator*= (LLColor4) — let's check what this does.
        // Header comment says: "Doesn't multiply alpha! (for lighting)"
        LLColor4 light_ll(1.0f, 0.8f, 0.6f, 0.7f);
        LLColor4 mat_ll(0.5f, 0.5f, 0.5f, 0.9f);
        LLColor4 result_ll = light_ll;
        result_ll *= mat_ll;

        // R/G/B should be product, alpha unchanged from light_ll
        ensure_approximately_equals("r product", result_ll.mV[VRED],   0.5f, 16);
        ensure_approximately_equals("g product", result_ll.mV[VGREEN], 0.4f, 16);
        ensure_approximately_equals("b product", result_ll.mV[VBLUE],  0.3f, 16);
        ensure_approximately_equals("a from light, NOT product",
                                    result_ll.mV[VALPHA], 0.7f, 16);

        // glm vec4 * vec4 component-wise multiplies ALL components.
        glm::vec4 light_gm(1.0f, 0.8f, 0.6f, 0.7f);
        glm::vec4 mat_gm(0.5f, 0.5f, 0.5f, 0.9f);
        glm::vec4 result_gm = light_gm * mat_gm;
        ensure_approximately_equals("glm a IS product", result_gm.w, 0.63f, 16);

        // Divergence on alpha:
        ensure("LL color *= and glm vec4 * diverge on alpha",
               std::fabs(result_ll.mV[VALPHA] - result_gm.w) > 1e-4f);
    }

    // ---------- Round-trip ----------

    template<> template<>
    void v4color_glm_equiv_object::test<9>()
    {
        LLColor4 original(0.123f, 0.456f, 0.789f, 0.321f);
        LLColor4 round = to_ll(to_glm(original));
        ensure("round-trip preserves rgba", color_near(round, to_glm(original)));
    }

    // ---------- Conversion to/from LLColor3 ----------

    template<> template<>
    void v4color_glm_equiv_object::test<10>()
    {
        // LLColor4 → LLColor3 (drop alpha) should match glm vec4 → vec3.
        LLColor4 ll4(0.5f, 0.6f, 0.7f, 0.8f);
        LLColor3 ll3(ll4);

        glm::vec4 gm4(0.5f, 0.6f, 0.7f, 0.8f);
        glm::vec3 gm3(gm4);

        ensure_approximately_equals("r matches", ll3.mV[0], gm3.x, 16);
        ensure_approximately_equals("g matches", ll3.mV[1], gm3.y, 16);
        ensure_approximately_equals("b matches", ll3.mV[2], gm3.z, 16);
    }
}
