#include <clingo.hh>
#include <clingo-dl.h>
#include "catch.hpp"

class Handler : public Clingo::SolveEventHandler {
public:
    Handler(clingodl_propagator_t *prop) : prop_{prop} { }
    void on_statistics(Clingo::UserStatistics step, Clingo::UserStatistics accu) override {
        clingodl_on_statistics(prop_, step.to_c(), accu.to_c());
    }

private:
    clingodl_propagator_t *prop_;
};

using ResultVec = std::vector<std::vector<std::pair<Clingo::Symbol, int>>>;
ResultVec solve(clingodl_propagator_t *prop, Clingo::Control &ctl) {
    Handler h{prop};
    using namespace Clingo;
    ResultVec result;
    for (auto &&m : ctl.solve(LiteralSpan{}, &h)) {
        result.emplace_back();
        auto &sol = result.back();
        auto id = m.thread_id();
        size_t index;
        for (clingodl_assignment_begin(prop, id, &index); clingodl_assignment_next(prop, id, &index); ) {
            clingodl_value_t value;
            clingodl_assignment_get_value(prop, id, index, &value);
            REQUIRE(value.type == clingodl_value_type_int);
            sol.emplace_back(Symbol{clingodl_get_symbol(prop, index)}, value.int_number);
        }
        std::sort(sol.begin(), sol.end());
    }
    std::sort(result.begin(), result.end());
    return result;
}

TEST_CASE("solving", "[clingo]") {
    SECTION("with control") {
        using namespace Clingo;
        auto a = Id("a"),  b = Id("b");
        Control ctl{{"0"}};
        clingodl_propagator_t *prop;
        REQUIRE(clingodl_create_propagator(&prop));
        SECTION("solve") {
            REQUIRE(clingodl_register_propagator(prop, ctl.to_c()));
            ctl.add("base", {},
                "1 { a; b } 1. &diff { a - b } <= 3.\n"
                "&diff { 0 - a } <= -5 :- a.\n"
                "&diff { 0 - b } <= -7 :- b.\n"
                "&show_assignment{}.\n");
            ctl.ground({{"base", {}}});
            auto result = solve(prop, ctl);
            REQUIRE(result == (ResultVec{{{a, 0}, {b, 7}}, {{a, 5}, {b, 2}}}));

            ctl.add("ext", {},
                "&diff { a - 0 } <= 4.\n");
            ctl.ground({{"ext", {}}});
            result = solve(prop, ctl);
            REQUIRE(result == (ResultVec{{{a, 0}, {b, 7}}}));
        }
        SECTION("configure") {
            REQUIRE(clingodl_configure_propagator(prop, "propagate", "full"));
            REQUIRE(clingodl_register_propagator(prop, ctl.to_c()));

            ctl.add("base", {},
                "&diff { a - 0 } <= 0.\n"
                "a :- &diff { a - 0 } <=  0.\n"
                "b :- &diff { 0 - a } <= -1.\n"
                );
            ctl.ground({{"base", {}}});

            auto result = solve(prop, ctl);
            REQUIRE(result == (ResultVec{{{a, 0}}}));

            REQUIRE(ctl.statistics()["user_accu"]["DifferenceLogic"]["Thread"][(size_t)0]["Edges propagated"].value() >= 1);
        }
        clingodl_destroy_propagator(prop);
    }
}
