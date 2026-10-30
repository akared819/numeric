#include "numeric.h"
#include <gsl/gsl_errno.h>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <limits>
#include <thread>
#include <vector>
using namespace numeric;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); return 1; } } while (0)
namespace {
int destroyed = 0;
class Lifetime : public Operation {
public:
    ~Lifetime() override { ++destroyed; }
    void reset() override {}
protected:
    Result executeImpl(const Input*, std::size_t, Output*, std::size_t) override { return {Status::Ok, "ok"}; }
};
std::unique_ptr<Operation> makeLifetime() { return std::unique_ptr<Operation>(new Lifetime); }
}
int main() {
    Registry r; registerBuiltins(r);
    auto op = r.create("mean"); CHECK(op && r.find("mean") && !r.find("missing") && !r.create("missing"));
    double data[] = {1, 99, 3, 99, 5}; double result = -1;
    Input in{data, 3, 5, 2, Type::Float64, Shape::Array};
    Output out{&result, 1, 7, Type::Float64, Shape::Scalar};
    CHECK(op->execute(&in, 1, &out, 1).status == Status::NotInitialized && out.count == 0 && result == -1);
    initializeGsl(); initializeGsl(); gsl_error("test", __FILE__, __LINE__, GSL_EDOM);
    CHECK(op->execute(&in, 1, &out, 1).status == Status::Ok && out.count == 1 && std::fabs(result - 3) < 1e-12);
    in.count = 1; CHECK(op->execute(&in, 1, &out, 1).status == Status::Ok && result == 1);
    result = 42; in.count = 0; CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument && out.count == 0 && result == 42);
    in.count = 3; in.capacity = 4; CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument && result == 42);
    in.capacity = 5; in.stride = 0; CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    in.stride = std::numeric_limits<std::size_t>::max(); CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    in.stride = 2; out.capacity = 0; CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    out.capacity = 1; in.shape = Shape::Scalar; CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    in.shape = Shape::Array; in.data = nullptr; CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    in.data = data; out.data = nullptr; CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument); out.data = &result;
    data[2] = std::numeric_limits<double>::quiet_NaN(); CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    data[2] = std::numeric_limits<double>::infinity(); CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    data[2] = 3; op->reset(); CHECK(op->execute(&in, 1, &out, 1).status == Status::Ok);
    CHECK(op->execute(nullptr, 0, &out, 1).status == Status::InvalidArgument);
    double extreme[] = {std::numeric_limits<double>::max(), -std::numeric_limits<double>::max()};
    Input bad{extreme, 2, 2, 1, Type::Float64, Shape::Array}; result = 42;
    Result extremeResult = op->execute(&bad, 1, &out, 1);
    CHECK((extremeResult.status == Status::NumericalError && out.count == 0 && result == 42) ||
          (extremeResult.status == Status::Ok && out.count == 1 && result == 0));
    Metadata meta = *r.find("mean"); meta.name = "lifetime";
    CHECK(r.add(meta, &makeLifetime)); meta.name = "changed"; meta.category.clear();
    CHECK(r.find("lifetime") && r.find("lifetime")->category == "statistics");
    CHECK(!r.add(*r.find("lifetime"), &makeLifetime));
    CHECK(!r.add(meta, &makeLifetime));
    meta.category = "test"; meta.name = "bad"; meta.inputs[0].shape = static_cast<Shape>(99);
    CHECK(!r.add(meta, &makeLifetime));
    { auto owned = r.create("lifetime"); CHECK(owned && destroyed == 0); }
    CHECK(destroyed == 1);
    std::atomic<bool> good(true); std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) threads.emplace_back([&r, &good] {
        auto instance = r.create("mean"); double values[] = {1, 2, 3}, y = 0;
        Input x{values, 3, 3, 1, Type::Float64, Shape::Array}; Output o{&y, 1, 0, Type::Float64, Shape::Scalar};
        for (int i = 0; i < 1000; ++i)
            if (instance->execute(&x, 1, &o, 1).status != Status::Ok || y != 2) good = false;
    });
    for (auto& thread : threads) thread.join(); CHECK(good.load());
    std::puts("numericTest: all checks passed"); return 0;
}
