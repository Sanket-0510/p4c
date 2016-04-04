#include "../../lib/path.h"
#include "../../lib/exceptions.h"
#include "../../lib/stringref.h"
#include "test.h"

using namespace Util;

namespace Test {
class TestPath : public TestBase {
    int testPath() {
#if _WIN32
        throw std::logic_error("Tests for WIN32 not yet written");
#endif        
        {
            PathName path = "/usr/local/bin/file.exe";
            StringRef ext = path.getExtension();
            ASSERT_EQ(ext, StringRef("exe"));

            PathName file = path.getFilename();
            ASSERT_EQ(file.toString(), "file.exe");

            StringRef base = path.getBasename();
            ASSERT_EQ(base, StringRef("file"));

            PathName folder = path.getFolder();
            ASSERT_EQ(folder, "/usr/local/bin");
        }

        {
            PathName path = "/usr/local/bin/";
            StringRef ext = path.getExtension();
            ASSERT_EQ(ext, "");

            PathName file = path.getFilename();
            ASSERT_EQ(file.toString(), "");

            StringRef base = path.getBasename();
            ASSERT_EQ(base, "");

            PathName folder = path.getFolder();
            ASSERT_EQ(folder, "/usr/local/bin");
        }

        {
            PathName path = "file.exe";
            StringRef ext = path.getExtension();
            ASSERT_EQ(ext, StringRef("exe"));

            PathName file = path.getFilename();
            ASSERT_EQ(file.toString(), "file.exe");

            StringRef base = path.getBasename();
            ASSERT_EQ(base, StringRef("file"));

            PathName folder = path.getFolder();
            ASSERT_EQ(folder, "");
        }

        {
            PathName path = "";
            ASSERT_EQ(path.isNullOrEmpty(), true);
            PathName grow = path.join("x");
            ASSERT_EQ(grow.toString(), "x");
            ASSERT_EQ(grow.isNullOrEmpty(), false);

            grow = grow.join("y");
            ASSERT_EQ(grow.toString(), "x/y");

            path = PathName("x/");
            grow = path.join("y");
            ASSERT_EQ(grow.toString(), "x/y");
        }
        
        return SUCCESS;
    }

 public:
    int run() {
        RUNTEST(testPath);
        return SUCCESS;
    }
};
}  // namespace Test


int main(int, char* []) {
    Test::TestPath test;
    return test.run();
}
