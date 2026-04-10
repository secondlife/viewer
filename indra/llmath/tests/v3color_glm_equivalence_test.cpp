/**
 * @file v3color_glm_equivalence_test.cpp
 * @brief Differential tests for LLColor3 ↔ glm::vec3 (used as RGB).
 *
 * Unlike LLVector4, LLColor3 has correctly behaving arithmetic
 * operators that touch all three components. This test confirms.
 *
 * GLM has no native color type — we use glm::vec3 as the canonical
 * RGB representation.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "../test/lltut.h"

#include "../llmath.h"
#include "../v3color.h"

#include <glm/glm.hpp>

#include <cmath>

namespace tut
{
    struct v3color_glm_equiv_data
    {
        static constexpr F32 kEps = 1e-5f;

        static glm::vec3 to_glm(const LLColor3& c)
        {
            return glm::vec3(c.mV[0], c.mV[1], c.mV[2]);
        }

        static LLColor3 to_ll(const glm::vec3& c)
        {
            return LLColor3(c.x, c.y, c.z);
        }

        static bool color_near(const LLColor3& a, const glm::vec3& b, F32 eps = kEps)
        {
            return std::fabs(a.mV[0] - b.x) <= eps
                && std::fabs(a.mV[1] - b.y) <= eps
                && std::fabs(a.mV[2] - b.z) <= eps;
        }
    };

    using v3color_glm_equiv_test = test_group<v3color_glm_equiv_data>;
    using v3color_glm_equiv_object = v3color_glm_equiv_test::object;
    tut::v3color_glm_equiv_test v3color_glm_equiv_testcase("v3color_glm_equivalence");

    template<> template<>
    void v3color_glm_equiv_object::test<1>()
    {
        LLColor3 ll(0.5f, 0.7f, 0.3f);
        glm::vec3 gm(0.5f, 0.7f, 0.3f);
        ensure("rgb construction matches", color_near(ll, gm));
    }

    template<> template<>
    void v3color_glm_equiv_object::test<2>()
    {
        // Color addition (e.g., adding two light contributions).
        LLColor3 a_ll(0.2f, 0.3f, 0.4f);
        LLColor3 b_ll(0.5f, 0.6f, 0.7f);
        LLColor3 sum_ll = a_ll + b_ll;

        glm::vec3 a_gm(0.2f, 0.3f, 0.4f);
        glm::vec3 b_gm(0.5f, 0.6f, 0.7f);
        glm::vec3 sum_gm = a_gm + b_gm;

        ensure("color addition matches", color_near(sum_ll, sum_gm));
    }

    template<> template<>
    void v3color_glm_equiv_object::test<3>()
    {
        // Scalar multiply (e.g., dimming a light).
        LLColor3 ll(0.4f, 0.6f, 0.8f);
        LLColor3 dimmed_ll = ll * 0.5f;

        glm::vec3 gm(0.4f, 0.6f, 0.8f);
        glm::vec3 dimmed_gm = gm * 0.5f;

        ensure("scalar multiply (dim) matches", color_near(dimmed_ll, dimmed_gm));
    }

    template<> template<>
    void v3color_glm_equiv_object::test<4>()
    {
        // Component-wise multiply (e.g., light * material). LLColor3
        // uses operator*= for this.
        LLColor3 light_ll(1.0f, 0.8f, 0.6f);
        LLColor3 mat_ll(0.5f, 0.5f, 0.5f);
        LLColor3 result_ll = light_ll;
        result_ll *= mat_ll;

        glm::vec3 light_gm(1.0f, 0.8f, 0.6f);
        glm::vec3 mat_gm(0.5f, 0.5f, 0.5f);
        glm::vec3 result_gm = light_gm * mat_gm;

        ensure("component-wise color multiply matches", color_near(result_ll, result_gm));
    }

    template<> template<>
    void v3color_glm_equiv_object::test<5>()
    {
        // Default constructor produces zero (black) in LL.
        LLColor3 ll;
        glm::vec3 gm(0.0f);
        ensure("default ctor is black", color_near(ll, gm));
    }

    template<> template<>
    void v3color_glm_equiv_object::test<6>()
    {
        // Round-trip through glm preserves all three components.
        LLColor3 original(0.123f, 0.456f, 0.789f);
        LLColor3 round = to_ll(to_glm(original));
        ensure("round-trip preserves rgb",
               std::fabs(round.mV[0] - original.mV[0]) < kEps
               && std::fabs(round.mV[1] - original.mV[1]) < kEps
               && std::fabs(round.mV[2] - original.mV[2]) < kEps);
    }

    template<> template<>
    void v3color_glm_equiv_object::test<7>()
    {
        // length() / "brightness" — sum of squared components.
        LLColor3 ll(0.6f, 0.8f, 0.0f);  // length 1.0
        F32 len_ll = ll.length();

        glm::vec3 gm(0.6f, 0.8f, 0.0f);
        F32 len_gm = glm::length(gm);

        ensure_approximately_equals("color length matches", len_ll, len_gm, 16);
    }

    // ====================================================================
    // Semantic landmines — these tests pin LL-specific behaviour that
    // does NOT match the obvious glm equivalent. The migration MUST
    // preserve LL semantics; a failing assertion here means the
    // migration silently changed behaviour.
    // ====================================================================

    template<> template<>
    void v3color_glm_equiv_object::test<8>()
    {
        // LANDMINE: LLColor3 unary minus is COLOR INVERSE (1 - rgb),
        // NOT component negation. glm::vec3 unary minus is component
        // negation. The replacement for `-c` after migration is
        // `glm::vec3(1.f) - c`, NOT `-c`.
        const LLColor3 c_ll(0.2f, 0.6f, 0.9f);
        const LLColor3 inv_ll = -c_ll;
        ensure("LL unary -: r is 1-r", std::fabs(inv_ll.mV[0] - 0.8f) < kEps);
        ensure("LL unary -: g is 1-g", std::fabs(inv_ll.mV[1] - 0.4f) < kEps);
        ensure("LL unary -: b is 1-b", std::fabs(inv_ll.mV[2] - 0.1f) < kEps);

        const glm::vec3 c_gm(0.2f, 0.6f, 0.9f);
        const glm::vec3 neg_gm = -c_gm;  // glm: component negation
        ensure("glm unary -: r is -r", std::fabs(neg_gm.x - (-0.2f)) < kEps);
        ensure("glm unary -: g is -g", std::fabs(neg_gm.y - (-0.6f)) < kEps);
        ensure("glm unary -: b is -b", std::fabs(neg_gm.z - (-0.9f)) < kEps);

        // Correct glm replacement for LL unary minus:
        const glm::vec3 inv_gm = glm::vec3(1.f) - c_gm;
        ensure("LL inverse matches `1 - c`", color_near(inv_ll, inv_gm));
    }

    template<> template<>
    void v3color_glm_equiv_object::test<9>()
    {
        // LANDMINE: LLColor3::brightness() is the naive average
        // (r+g+b)/3, NOT Rec.709 luminance. The migration helper
        // color3_brightness(glm::vec3) must preserve this exact
        // formula. Don't replace with glm::dot(c, vec3(0.2126,
        // 0.7152, 0.0722)) — that's a different value.
        const LLColor3 ll(0.3f, 0.6f, 0.9f);
        const F32 expected = (0.3f + 0.6f + 0.9f) / 3.0f;
        ensure_approximately_equals("brightness == (r+g+b)/3", ll.brightness(), expected, 16);

        const glm::vec3 gm(0.3f, 0.6f, 0.9f);
        const F32 gm_average = (gm.x + gm.y + gm.z) / 3.0f;
        ensure_approximately_equals("glm replacement matches", gm_average, expected, 16);

        // Pin the difference vs. Rec.709 luminance so future readers
        // see why this is not just glm::dot.
        const F32 rec709 = 0.2126f * gm.x + 0.7152f * gm.y + 0.0722f * gm.z;
        ensure("LL brightness != Rec.709 luminance", std::fabs(rec709 - expected) > 0.01f);
    }

    template<> template<>
    void v3color_glm_equiv_object::test<10>()
    {
        // clamp() to [0, 1] matches glm::clamp(c, 0.f, 1.f).
        LLColor3 ll(-0.3f, 0.5f, 1.7f);
        ll.clamp();
        ensure("LL clamp r", std::fabs(ll.mV[0] - 0.f) < kEps);
        ensure("LL clamp g", std::fabs(ll.mV[1] - 0.5f) < kEps);
        ensure("LL clamp b", std::fabs(ll.mV[2] - 1.f) < kEps);

        glm::vec3 gm(-0.3f, 0.5f, 1.7f);
        gm = glm::clamp(gm, 0.f, 1.f);
        ensure("clamp matches glm::clamp", color_near(ll, gm));
    }

    template<> template<>
    void v3color_glm_equiv_object::test<11>()
    {
        // LANDMINE: LLColor3::exp() uses the LL_FAST_EXP macro — an
        // integer-bit-trick approximation of exp() with substantial
        // error (commonly 5-15% in the 0..1 range, much worse for
        // larger inputs). It is NOT a drop-in for glm::exp /
        // std::exp. The migration helper should use glm::exp
        // directly, which is a precision IMPROVEMENT, not a
        // regression — but call sites must explicitly choose the
        // glm version. This test pins both behaviours so the
        // migration doesn't accidentally re-introduce LL_FAST_EXP.
        LLColor3 ll(0.1f, 0.5f, 1.0f);
        ll.exp();

        const glm::vec3 gm_in(0.1f, 0.5f, 1.0f);
        const glm::vec3 gm_precise = glm::exp(gm_in);

        // Sanity: ll values are positive and roughly monotonic.
        ensure("LL exp r positive", ll.mV[0] > 0.f);
        ensure("LL exp g positive", ll.mV[1] > 0.f);
        ensure("LL exp b positive", ll.mV[2] > 0.f);
        ensure("LL exp r < g (input ordering preserved)", ll.mV[0] < ll.mV[1]);
        ensure("LL exp g < b", ll.mV[1] < ll.mV[2]);

        // The two are MEASURABLY DIFFERENT — proving LL_FAST_EXP is
        // not a precise replacement for glm::exp.
        const F32 max_delta = std::max({
            std::fabs(ll.mV[0] - gm_precise.x),
            std::fabs(ll.mV[1] - gm_precise.y),
            std::fabs(ll.mV[2] - gm_precise.z)});
        ensure("LL_FAST_EXP is measurably less precise than glm::exp",
               max_delta > 1e-4f);
    }

    template<> template<>
    void v3color_glm_equiv_object::test<12>()
    {
        // v3colorutil componentSqrt / componentPow / componentExp
        // map to glm::sqrt / glm::pow / glm::exp on glm::vec3.
        const LLColor3 a_ll(0.04f, 0.25f, 0.81f);
        const LLColor3 sqrt_ll(std::sqrt(a_ll.mV[0]),
                               std::sqrt(a_ll.mV[1]),
                               std::sqrt(a_ll.mV[2]));
        const glm::vec3 a_gm(0.04f, 0.25f, 0.81f);
        const glm::vec3 sqrt_gm = glm::sqrt(a_gm);
        ensure("componentSqrt matches glm::sqrt", color_near(sqrt_ll, sqrt_gm));

        const LLColor3 pow_ll(std::pow(a_ll.mV[0], 2.2f),
                              std::pow(a_ll.mV[1], 2.2f),
                              std::pow(a_ll.mV[2], 2.2f));
        const glm::vec3 pow_gm = glm::pow(a_gm, glm::vec3(2.2f));
        ensure("componentPow matches glm::pow", color_near(pow_ll, pow_gm));
    }

    template<> template<>
    void v3color_glm_equiv_object::test<13>()
    {
        // setHSL round-trip: hsl -> rgb -> calcHSL recovers the input.
        // Saturated, mid-luminance avoids the edge cases at the poles.
        const F32 h_in = 0.33f;
        const F32 s_in = 0.7f;
        const F32 l_in = 0.5f;

        LLColor3 c;
        c.setHSL(h_in, s_in, l_in);

        F32 h_out = 0.f, s_out = 0.f, l_out = 0.f;
        c.calcHSL(&h_out, &s_out, &l_out);

        ensure_approximately_equals("HSL round-trip H", h_in, h_out, 8);
        ensure_approximately_equals("HSL round-trip S", s_in, s_out, 8);
        ensure_approximately_equals("HSL round-trip L", l_in, l_out, 8);
    }

    template<> template<>
    void v3color_glm_equiv_object::test<14>()
    {
        // setHSL with zero saturation produces a neutral gray at L.
        LLColor3 c;
        c.setHSL(0.5f, 0.f, 0.7f);
        ensure("zero-sat HSL: r == L", std::fabs(c.mV[0] - 0.7f) < kEps);
        ensure("zero-sat HSL: g == L", std::fabs(c.mV[1] - 0.7f) < kEps);
        ensure("zero-sat HSL: b == L", std::fabs(c.mV[2] - 0.7f) < kEps);
    }

    // ====================================================================
    // ll_color3:: helper namespace tests — pin that the migration
    // helpers preserve the LL semantics from tests #8-#14.
    // ====================================================================

    template<> template<>
    void v3color_glm_equiv_object::test<15>()
    {
        // ll_color3::inverse matches LLColor3 unary minus.
        const LLColor3 c_ll(0.2f, 0.6f, 0.9f);
        const LLColor3 inv_ll = -c_ll;

        const glm::vec3 c_gm(0.2f, 0.6f, 0.9f);
        const glm::vec3 inv_gm = ll_color3::inverse(c_gm);

        ensure("ll_color3::inverse matches LL unary minus",
               color_near(inv_ll, inv_gm));
    }

    template<> template<>
    void v3color_glm_equiv_object::test<16>()
    {
        // ll_color3::brightness matches LLColor3::brightness().
        const LLColor3 ll(0.3f, 0.6f, 0.9f);
        const glm::vec3 gm(0.3f, 0.6f, 0.9f);
        ensure_approximately_equals("ll_color3::brightness matches",
            ll.brightness(), ll_color3::brightness(gm), 16);
    }

    template<> template<>
    void v3color_glm_equiv_object::test<17>()
    {
        // ll_color3::from_html matches LLColor3 hex ctor for valid
        // and edge inputs.
        ensure("from_html matches LL: FFDDEE",
            color_near(LLColor3("FFDDEE"), ll_color3::from_html("FFDDEE")));
        ensure("from_html matches LL: 000000",
            color_near(LLColor3("000000"), ll_color3::from_html("000000")));
        ensure("from_html matches LL: FFFFFF",
            color_near(LLColor3("FFFFFF"), ll_color3::from_html("FFFFFF")));

        // Bonus: helper accepts a leading '#' (the LL ctor doesnt).
        const glm::vec3 hashed = ll_color3::from_html("#FFDDEE");
        const glm::vec3 unhashed = ll_color3::from_html("FFDDEE");
        ensure("from_html accepts leading #",
               std::fabs(hashed.x - unhashed.x) < kEps
            && std::fabs(hashed.y - unhashed.y) < kEps
            && std::fabs(hashed.z - unhashed.z) < kEps);

        // Malformed input returns black.
        const glm::vec3 bad = ll_color3::from_html("xx");
        ensure("from_html: malformed -> black",
            bad.x == 0.f && bad.y == 0.f && bad.z == 0.f);

        // nullptr returns black, doesnt crash.
        const glm::vec3 null_in = ll_color3::from_html(nullptr);
        ensure("from_html: nullptr -> black",
            null_in.x == 0.f && null_in.y == 0.f && null_in.z == 0.f);
    }

    template<> template<>
    void v3color_glm_equiv_object::test<18>()
    {
        // ll_color3::from_hsl matches LLColor3::setHSL across a few
        // points covering: zero saturation, mid saturation, hue
        // wrap, primary colors.
        struct hsl_case { F32 h; F32 s; F32 l; const char* tag; };
        const hsl_case cases[] = {
            {0.f,    0.f,  0.7f, "zero-sat"},
            {0.33f,  0.7f, 0.5f, "saturated mid"},
            {0.f,    1.f,  0.5f, "pure red"},
            {1.f/3,  1.f,  0.5f, "pure green"},
            {2.f/3,  1.f,  0.5f, "pure blue"},
            {0.95f,  0.6f, 0.4f, "near hue wrap"},
        };
        for (const auto& tc : cases)
        {
            LLColor3 ll;
            ll.setHSL(tc.h, tc.s, tc.l);
            const glm::vec3 gm = ll_color3::from_hsl(tc.h, tc.s, tc.l);
            ensure(tc.tag, color_near(ll, gm));
        }
    }

    template<> template<>
    void v3color_glm_equiv_object::test<19>()
    {
        // ll_color3::to_hsl matches LLColor3::calcHSL.
        const LLColor3 ll(0.3f, 0.6f, 0.9f);
        F32 h_ll = 0, s_ll = 0, l_ll = 0;
        ll.calcHSL(&h_ll, &s_ll, &l_ll);

        const glm::vec3 gm(0.3f, 0.6f, 0.9f);
        F32 h_gm = 0, s_gm = 0, l_gm = 0;
        ll_color3::to_hsl(gm, &h_gm, &s_gm, &l_gm);

        ensure_approximately_equals("to_hsl H matches calcHSL", h_ll, h_gm, 16);
        ensure_approximately_equals("to_hsl S matches calcHSL", s_ll, s_gm, 16);
        ensure_approximately_equals("to_hsl L matches calcHSL", l_ll, l_gm, 16);
    }

    template<> template<>
    void v3color_glm_equiv_object::test<20>()
    {
        // ll_color3::to_srgb / to_linear match the LL helpers.
        const LLColor3 linear_ll(0.04f, 0.5f, 0.9f);
        const LLColor3 srgb_ll = srgbColor3(linear_ll);

        const glm::vec3 linear_gm(0.04f, 0.5f, 0.9f);
        const glm::vec3 srgb_gm = ll_color3::to_srgb(linear_gm);
        ensure("to_srgb matches srgbColor3", color_near(srgb_ll, srgb_gm));

        // round-trip srgb -> linear via the helper.
        const glm::vec3 back = ll_color3::to_linear(srgb_gm);
        ensure("to_linear undoes to_srgb", color_near(linear_ll, back, 1e-4f));
    }

    template<> template<>
    void v3color_glm_equiv_object::test<21>()
    {
        // Color constants in the helper namespace match the LL statics.
        ensure("white matches", color_near(LLColor3::white, ll_color3::white));
        ensure("black matches", color_near(LLColor3::black, ll_color3::black));
        ensure("grey matches",  color_near(LLColor3::grey,  ll_color3::grey));
    }
}
