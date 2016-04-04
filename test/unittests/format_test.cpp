#include "../../lib/error.h"
#include "../../lib/cstring.h"
#include "../../lib/stringify.h"
#include "test.h"

using namespace Util;

namespace Test {
class NiceFormat {
 private:
    int a, b, c;

 public:
    NiceFormat(int a, int b, int c) :
            a(a),
            b(b),
            c(c) {}

    cstring toString() const {
        return cstring("(") +
                Util::toString(this->a) + "," +
                Util::toString(this->b) + "," +
                Util::toString(this->c) + ")";
    }
};

class TestFormat : public TestBase {
 public:
    int run() {
        cstring message = ErrorReporter::instance.format_message("%1%", 5u);
        ASSERT_EQ(message, "5\n");

        message = ErrorReporter::instance.format_message("Number=%1%", 5);
        ASSERT_EQ(message, "Number=5\n");

        message = ErrorReporter::instance.format_message("Double=%1% String=%2%", 2.3, "short");
        ASSERT_EQ(message, "Double=2.3 String=short\n");

        NiceFormat nf(1, 2, 3);
        message = ErrorReporter::instance.format_message("Nice=%1%", nf);
        ASSERT_EQ(message, "Nice=(1,2,3)\n");

        cstring x = "x";
        cstring y = "y";
        message = ErrorReporter::instance.format_message("%1% %2%", x, y);
        ASSERT_EQ(message, "x y\n");

        message = ErrorReporter::instance.format_message("%1% %2%", x, 5);
        ASSERT_EQ(message, "x 5\n");

        message = Util::printf_format("Number=%d, String=%s", 5, "short");
        ASSERT_EQ(message, "Number=5, String=short");

        return SUCCESS;
    }
};
}  // namespace Test

int main(int /*argc*/, char* /*argv*/[]) {
    Test::TestFormat test;
    return test.run();
}

