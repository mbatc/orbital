#include "core/Vector.h"
#include "framework/test.h"

BFC_TEST(Vector_DefaultConstruct)
{
  bfc::Vector<int> a;
  BFC_TEST_ASSERT_TRUE(a.size() == 0);
  BFC_TEST_ASSERT_TRUE(a.empty());
}

BFC_TEST(Vector_ConstructSize)
{
  bfc::Vector<int> a(10, 0);
  BFC_TEST_ASSERT_TRUE(a.size() == 10);
  for (int64_t i = 0; i < a.size(); ++i)
    BFC_TEST_ASSERT_TRUE(a[i] == 0);

  bfc::Vector<int> b(5, 2);
  BFC_TEST_ASSERT_TRUE(b.size() == 5);
  for (int64_t i = 0; i < b.size(); ++i)
    BFC_TEST_ASSERT_TRUE(b[i] == 2);
}

BFC_TEST(Vector_At)
{
  bfc::Vector<int> a;
  for (int i = 0; i < 10; ++i)
    a.pushBack(i);

  BFC_TEST_ASSERT_TRUE(a.size() == 10);
  for (int i = 0; i < 10; ++i)
    BFC_TEST_ASSERT_TRUE(a.at(i) == i);
}

BFC_TEST(Vector_ArrayOperator)
{
  bfc::Vector<int> a;
  for (int i = 0; i < 10; ++i)
    a.pushBack(i);

  BFC_TEST_ASSERT_TRUE(a.size() == 10);
  for (int i = 0; i < 10; ++i)
    BFC_TEST_ASSERT_TRUE(a[i] == i);
}

BFC_TEST(Vector_PushBack)
{
  bfc::Vector<int> a;
  a.resize(5, 100);
  BFC_TEST_ASSERT_TRUE(a.size() == 5);
  BFC_TEST_ASSERT_TRUE(a.at(4) == 100);

  a.pushBack(3);
  BFC_TEST_ASSERT_TRUE(a.size() == 6);
  BFC_TEST_ASSERT_TRUE(a.at(4) == 100);
  BFC_TEST_ASSERT_TRUE(a.at(5) == 3);

  a.pushBack(2);
  BFC_TEST_ASSERT_TRUE(a.size() == 7);
  BFC_TEST_ASSERT_TRUE(a.at(4) == 100);
  BFC_TEST_ASSERT_TRUE(a.at(5) == 3);
  BFC_TEST_ASSERT_TRUE(a.at(6) == 2);

  a.pushBack(1);
  BFC_TEST_ASSERT_TRUE(a.size() == 8);
  BFC_TEST_ASSERT_TRUE(a.at(4) == 100);
  BFC_TEST_ASSERT_TRUE(a.at(5) == 3);
  BFC_TEST_ASSERT_TRUE(a.at(6) == 2);
  BFC_TEST_ASSERT_TRUE(a.at(7) == 1);

  a.pushBack(0);
  BFC_TEST_ASSERT_TRUE(a.size() == 9);
  BFC_TEST_ASSERT_TRUE(a.at(4) == 100);
  BFC_TEST_ASSERT_TRUE(a.at(5) == 3);
  BFC_TEST_ASSERT_TRUE(a.at(6) == 2);
  BFC_TEST_ASSERT_TRUE(a.at(7) == 1);
  BFC_TEST_ASSERT_TRUE(a.at(8) == 0);
}

BFC_TEST(Vector_PushFront)
{
  bfc::Vector<int> a;
  a.resize(5, 100);
  BFC_TEST_ASSERT_TRUE(a.size() == 5);
  BFC_TEST_ASSERT_TRUE(a.at(0) == 100);

  a.pushFront(3);
  BFC_TEST_ASSERT_TRUE(a.size() == 6);
  BFC_TEST_ASSERT_TRUE(a.at(0) == 3);
  BFC_TEST_ASSERT_TRUE(a.at(1) == 100);

  a.pushFront(2);
  BFC_TEST_ASSERT_TRUE(a.size() == 7);
  BFC_TEST_ASSERT_TRUE(a.at(0) == 2);
  BFC_TEST_ASSERT_TRUE(a.at(1) == 3);
  BFC_TEST_ASSERT_TRUE(a.at(2) == 100);

  a.pushFront(1);
  BFC_TEST_ASSERT_TRUE(a.size() == 8);
  BFC_TEST_ASSERT_TRUE(a.at(0) == 1);
  BFC_TEST_ASSERT_TRUE(a.at(1) == 2);
  BFC_TEST_ASSERT_TRUE(a.at(2) == 3);
  BFC_TEST_ASSERT_TRUE(a.at(3) == 100);

  a.pushFront(0);
  BFC_TEST_ASSERT_TRUE(a.size() == 9);
  BFC_TEST_ASSERT_TRUE(a.at(0) == 0);
  BFC_TEST_ASSERT_TRUE(a.at(1) == 1);
  BFC_TEST_ASSERT_TRUE(a.at(2) == 2);
  BFC_TEST_ASSERT_TRUE(a.at(3) == 3);
  BFC_TEST_ASSERT_TRUE(a.at(4) == 100);
}

BFC_TEST(Vector_Insert)
{
  bfc::Vector<int> a;
  BFC_TEST_ASSERT_TRUE(a.size() == 0);

  a.insert(0, 1);
  BFC_TEST_ASSERT_TRUE(a.size() == 1);
  BFC_TEST_ASSERT_TRUE(a.at(0) == 1);

  a.insert(0, 2);
  BFC_TEST_ASSERT_TRUE(a.size() == 2);
  BFC_TEST_ASSERT_TRUE(a.at(0) == 2);
  BFC_TEST_ASSERT_TRUE(a.at(1) == 1);

  a.insert(2, 3);
  BFC_TEST_ASSERT_TRUE(a.size() == 3);
  BFC_TEST_ASSERT_TRUE(a.at(0) == 2);
  BFC_TEST_ASSERT_TRUE(a.at(1) == 1);
  BFC_TEST_ASSERT_TRUE(a.at(2) == 3);

  a.insert(1, 4);
  BFC_TEST_ASSERT_TRUE(a.size() == 4);
  BFC_TEST_ASSERT_TRUE(a.at(0) == 2);
  BFC_TEST_ASSERT_TRUE(a.at(1) == 4);
  BFC_TEST_ASSERT_TRUE(a.at(2) == 1);
  BFC_TEST_ASSERT_TRUE(a.at(3) == 3);
}

BFC_TEST(Vector_InsertArray)
{
  int first[4] = { 0, 1, 2, 3 };
  int second[4] = { 4, 5, 6, 7 };

  bfc::Vector<int> a;
  BFC_TEST_ASSERT_TRUE(a.size() == 0);
  a.insert(0, first, first + 4);
  BFC_TEST_ASSERT_TRUE(a.size() == 4);
  BFC_TEST_ASSERT_TRUE(a.at(0) == 0);
  BFC_TEST_ASSERT_TRUE(a.at(1) == 1);
  BFC_TEST_ASSERT_TRUE(a.at(2) == 2);
  BFC_TEST_ASSERT_TRUE(a.at(3) == 3);

  a.insert(2, second, second + 4);
  BFC_TEST_ASSERT_TRUE(a.size() == 8);
  BFC_TEST_ASSERT_TRUE(a.at(0) == 0);
  BFC_TEST_ASSERT_TRUE(a.at(1) == 1);
  BFC_TEST_ASSERT_TRUE(a.at(2) == 4);
  BFC_TEST_ASSERT_TRUE(a.at(3) == 5);
  BFC_TEST_ASSERT_TRUE(a.at(4) == 6);
  BFC_TEST_ASSERT_TRUE(a.at(5) == 7);
  BFC_TEST_ASSERT_TRUE(a.at(6) == 2);
  BFC_TEST_ASSERT_TRUE(a.at(7) == 3);
}

BFC_TEST(Vector_Resize)
{
  bfc::Vector<int> a;
  a.resize(15, 3);

  BFC_TEST_ASSERT_TRUE(a.size() == 15);
  for (int64_t i = 0; i < a.size(); ++i)
    BFC_TEST_ASSERT_TRUE(a.at(i) == 3);

  a.resize(5, 10);
  BFC_TEST_ASSERT_TRUE(a.size() == 5);
  for (int64_t i = 0; i < a.size(); ++i)
    BFC_TEST_ASSERT_TRUE(a.at(i) == 3);


  a.resize(10, 12);
  BFC_TEST_ASSERT_TRUE(a.size() == 10);
  for (int64_t i = 0; i < a.size(); ++i)
    if (i < 5)
      BFC_TEST_ASSERT_TRUE(a.at(i) == 3);
    else
      BFC_TEST_ASSERT_TRUE(a.at(i) == 12);
}

BFC_TEST(Vector_Reserve)
{
  bfc::Vector<int> a;
  BFC_TEST_ASSERT_TRUE(a.size() == 0);
  BFC_TEST_ASSERT_TRUE(a.capacity() == 0);

  a.reserve(10);

  BFC_TEST_ASSERT_TRUE(a.size() == 0);
  BFC_TEST_ASSERT_TRUE(a.capacity() == 10);

  a.resize(5);

  BFC_TEST_ASSERT_TRUE(a.size() == 5);
  BFC_TEST_ASSERT_TRUE(a.capacity() == 10);

  a.reserve(15);

  BFC_TEST_ASSERT_TRUE(a.size() == 5);
  BFC_TEST_ASSERT_TRUE(a.capacity() == 15);
}
