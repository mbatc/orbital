#include "framework/test.h"
#include "core/Filename.h"

BFC_TEST(Filename_Parent) {
  bfc::Filename file = "D:/My/File/Name.exe";
  
  BFC_TEST_ASSERT_TRUE(file.parent() == "D:/My/File");
}

BFC_TEST(Filename_Name) {
  bfc::Filename file = "D:/My/File/Name.exe";
  bfc::StringView name = file.name();

  BFC_TEST_ASSERT_TRUE(file.name() == "Name.exe");
}

BFC_TEST(Filename_Extension) {
  bfc::Filename file = "D:/My/File/Name.exe";

  BFC_TEST_ASSERT_TRUE(file.extension() == "exe");
}

BFC_TEST(Filename_Drive) {
  bfc::Filename file = "D:/My/File/Name.exe";

  BFC_TEST_ASSERT_TRUE(file.drive() == "D");
}

BFC_TEST(Filename_Root) {
  bfc::Filename file = "D:/My/File/Name.exe";

  BFC_TEST_ASSERT_TRUE(file.root() == "D");

  bfc::Filename file2 = "/My/File/Name.exe";

  BFC_TEST_ASSERT_TRUE(file2.root() == "My");
}

BFC_TEST(Filename_GetDirect) {
  {
    bfc::Filename file = "D:/My/File/Name.exe";
    BFC_TEST_ASSERT_TRUE(file.getDirect() == file);
  }
  {
    bfc::Filename file = "D:/My/../File/Name.exe";
    BFC_TEST_ASSERT_TRUE(file.getDirect() == "D:/File/Name.exe");
  }
}
