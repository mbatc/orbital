#include "framework/test.h"
#include "core/RuntimeType.h"
//
//struct FirstComponent {
//  int a;
//  int b;
//};
//
//struct SecondComponent {
//  int a;
//  int b;
//};
//
//struct ThirdComponent {
//  int a;
//  int b;
//};
//
//BFC_DECLARE_COMPONENT(FirstComponent, "test.FirstComponent", 0);
//BFC_DECLARE_COMPONENT(SecondComponent, "test.SecondComponent", 0);
//BFC_DECLARE_COMPONENT(ThirdComponent, "test.ThirdComponent", 0);
//
//using namespace bfc;
//
//BFC_TEST(EntityManager_Construct) {
//  engine::Scene manager;
//
//  BFC_TEST_ASSERT_TRUE(manager.size() == 0);
//  BFC_TEST_ASSERT_TRUE(manager.capacity() == 0);
//}
//
//BFC_TEST(EntityManager_CreateEntity) {
//  EntityManager manager;
//
//  EntityID id = manager.createEntity();
//  BFC_TEST_ASSERT_TRUE(id != NullEntityID);
//  BFC_TEST_ASSERT_TRUE(manager.contains(id));
//}
//
//BFC_TEST(EntityManager_DeleteEntity) {
//  EntityManager manager;
//
//  EntityID id = manager.createEntity();
//  manager.deleteEntity(id);
//  BFC_TEST_ASSERT_FALSE(manager.contains(id));
//}
//
//BFC_TEST(EntityManager_AddSingleComponent) {
//  EntityManager manager;
//
//  EntityID id = manager.createEntity();
//
//  BFC_TEST_ASSERT_TRUE(manager.tryGet<FirstComponent>(id) == nullptr);
//
//  FirstComponent &newComponent = manager.add<FirstComponent>(id);
//
//  BFC_TEST_ASSERT_TRUE(manager.has<FirstComponent>(id));
//  BFC_TEST_ASSERT_TRUE(manager.tryGet<FirstComponent>(id) != nullptr);
//  BFC_TEST_ASSERT_TRUE(&newComponent == manager.tryGet<FirstComponent>(id));
//}
//
//BFC_TEST(EntityManager_RemoveSingleComponent) {
//  EntityManager manager;
//  EntityID id = manager.createEntity();
//  FirstComponent & newComponent = manager.add<FirstComponent>(id);
//  manager.remove<FirstComponent>(id);
//  BFC_TEST_ASSERT_FALSE(manager.has<FirstComponent>(id));
//  BFC_TEST_ASSERT_TRUE(manager.tryGet<FirstComponent>(id) == nullptr);
//}
//
//BFC_TEST(EntityManager_AddMultpleComponents) {
//  EntityManager manager;
//
//  EntityID id = manager.createEntity();
//
//  BFC_TEST_ASSERT_TRUE(manager.tryGet<FirstComponent>(id) == nullptr);
//  BFC_TEST_ASSERT_TRUE(manager.tryGet<SecondComponent>(id) == nullptr);
//  BFC_TEST_ASSERT_TRUE(manager.tryGet<ThirdComponent>(id) == nullptr);
//
//  FirstComponent &  first  = manager.add<FirstComponent>(id);
//  SecondComponent & second = manager.add<SecondComponent>(id);
//  ThirdComponent &  third  = manager.add<ThirdComponent>(id);
//
//  BFC_TEST_ASSERT_TRUE(manager.has<FirstComponent>(id));
//  BFC_TEST_ASSERT_TRUE(manager.has<SecondComponent>(id));
//  BFC_TEST_ASSERT_TRUE(manager.has<ThirdComponent>(id));
//
//  BFC_TEST_ASSERT_TRUE(manager.tryGet<FirstComponent>(id) != nullptr);
//  BFC_TEST_ASSERT_TRUE(manager.tryGet<SecondComponent>(id) != nullptr);
//  BFC_TEST_ASSERT_TRUE(manager.tryGet<ThirdComponent>(id) != nullptr);
//}
//
//BFC_TEST(EntityManager_RemoveMultipleComponents) {
//  EntityManager     manager;
//  EntityID          id     = manager.createEntity();
//  FirstComponent &  first  = manager.add<FirstComponent>(id);
//  SecondComponent & second = manager.add<SecondComponent>(id);
//  ThirdComponent &  third  = manager.add<ThirdComponent>(id);
//
//  manager.remove<FirstComponent>(id);
//  BFC_TEST_ASSERT_FALSE(manager.has<FirstComponent>(id));
//  BFC_TEST_ASSERT_TRUE(manager.has<SecondComponent>(id));
//  BFC_TEST_ASSERT_TRUE(manager.has<ThirdComponent>(id));
//
//  manager.remove<SecondComponent>(id);
//  BFC_TEST_ASSERT_FALSE(manager.has<FirstComponent>(id));
//  BFC_TEST_ASSERT_FALSE(manager.has<SecondComponent>(id));
//  BFC_TEST_ASSERT_TRUE(manager.has<ThirdComponent>(id));
//
//  manager.remove<ThirdComponent>(id);
//  BFC_TEST_ASSERT_FALSE(manager.has<FirstComponent>(id));
//  BFC_TEST_ASSERT_FALSE(manager.has<SecondComponent>(id));
//  BFC_TEST_ASSERT_FALSE(manager.has<ThirdComponent>(id));
//}
//
//BFC_TEST(EntityManager_DeleteEntityWithComponents) {
//  EntityManager manager;
//  EntityID id = manager.createEntity();
//  manager.add<FirstComponent>(id);
//  manager.add<SecondComponent>(id);
//  manager.add<ThirdComponent>(id);
//  manager.deleteEntity(id);
//
//  BFC_TEST_ASSERT_FALSE(manager.has<FirstComponent>(id));
//  BFC_TEST_ASSERT_FALSE(manager.has<SecondComponent>(id));
//  BFC_TEST_ASSERT_FALSE(manager.has<ThirdComponent>(id));
//}
//
//BFC_TEST(EntityManager_CreateMultipleEntities) {
//  EntityManager manager;
//  Vector<EntityID> ids;
//  for (int64_t i = 0; i < 100; ++i) {
//    ids.pushBack(manager.createEntity());
//  }
//
//  for (EntityID id : ids) {
//    BFC_TEST_ASSERT_TRUE(manager.contains(id));
//  }
//}
//
//BFC_TEST(EntityManager_RemoveMultipleEntities) {
//  EntityManager    manager;
//  Vector<EntityID> ids;
//  for (int64_t i = 0; i < 100; ++i) {
//    ids.pushBack(manager.createEntity());
//  }
//
//  // Remove entities in a random order.
//  Vector<EntityID> remainingIDs = ids;
//  Vector<EntityID> removedIDs;
//  while (remainingIDs.size() > 0) {
//    int64_t nextToRemove = rand() % remainingIDs.size();
//
//    // Remove the entity
//    manager.deleteEntity(remainingIDs[nextToRemove]);
//    removedIDs.pushBack(remainingIDs[nextToRemove]);
//    remainingIDs.erase(nextToRemove);
//
//    // Make sure all entities are correct (they either exist or don't exist)
//    for (EntityID removed : removedIDs) {
//      BFC_TEST_ASSERT_FALSE(manager.contains(removed));
//    }
//    for (EntityID remaining : remainingIDs) {
//      BFC_TEST_ASSERT_TRUE(manager.contains(remaining));
//    }
//  }
//}
//
//BFC_TEST(EntityManager_CreateMultipleEntitiesWithComponents) {
//  EntityManager    manager;
//
//  struct TestEntityInfo {
//    EntityID id;
//    bool hasFirst;
//    bool hasSecond;
//    bool hasThird;
//  };
//
//  Vector<TestEntityInfo> infos;
//
//  for (int64_t i = 0; i < 100; ++i) {
//    TestEntityInfo info;
//    info.id = manager.createEntity();
//    info.hasFirst = rand() % 2 == 0;
//    info.hasSecond = rand() % 2 == 0;
//    info.hasThird  = rand() % 2 == 0;
//
//    if (info.hasFirst)
//      manager.add<FirstComponent>(info.id);
//    if (info.hasSecond)
//      manager.add<SecondComponent>(info.id);
//    if (info.hasThird)
//      manager.add<ThirdComponent>(info.id);
//  }
//
//  for (TestEntityInfo info : infos) {
//    BFC_TEST_ASSERT_TRUE(manager.contains(info.id));
//    BFC_TEST_ASSERT_EQUAL(info.hasFirst, manager.has<FirstComponent>(info.id));
//    BFC_TEST_ASSERT_EQUAL(info.hasSecond, manager.has<SecondComponent>(info.id));
//    BFC_TEST_ASSERT_EQUAL(info.hasThird, manager.has<ThirdComponent>(info.id));
//  }
//}
//
//BFC_TEST(EntityManager_RemoveMultipleEntitiesWithComponents) {
//  EntityManager manager;
//
//  struct TestEntityInfo {
//    EntityID id;
//    bool     hasFirst;
//    bool     hasSecond;
//    bool     hasThird;
//  };
//
//  Vector<TestEntityInfo> infos;
//
//  // Create entities and a random set of components.
//  for (int64_t i = 0; i < 100; ++i) {
//    TestEntityInfo info;
//    info.id        = manager.createEntity();
//    info.hasFirst  = rand() % 2 == 0;
//    info.hasSecond = rand() % 2 == 0;
//    info.hasThird  = rand() % 2 == 0;
//
//    if (info.hasFirst)
//      manager.add<FirstComponent>(info.id);
//    if (info.hasSecond)
//      manager.add<SecondComponent>(info.id);
//    if (info.hasThird)
//      manager.add<ThirdComponent>(info.id);
//  }
//
//  // Remove entities in a random order.
//  Vector<TestEntityInfo> remainingIDs = infos;
//  Vector<TestEntityInfo> removedIDs;
//  while (remainingIDs.size() > 0) {
//    // Remove a random entity
//    int64_t        nextToRemove = rand() % remainingIDs.size();
//    TestEntityInfo remaining    = remainingIDs[nextToRemove];
//    manager.deleteEntity(remaining.id);
//    removedIDs.pushBack(remaining);
//    remainingIDs.erase(nextToRemove);
//
//    // Make sure all entities are correct (they either exist or don't exist)
//    for (TestEntityInfo info : removedIDs) {
//      BFC_TEST_ASSERT_FALSE(manager.contains(info.id));
//      BFC_TEST_ASSERT_FALSE(manager.has<FirstComponent>(info.id));
//      BFC_TEST_ASSERT_FALSE(manager.has<SecondComponent>(info.id));
//      BFC_TEST_ASSERT_FALSE(manager.has<ThirdComponent>(info.id));
//    }
//
//    for (TestEntityInfo info : remainingIDs) {
//      BFC_TEST_ASSERT_TRUE(manager.contains(info.id));
//      BFC_TEST_ASSERT_EQUAL(info.hasFirst, manager.has<FirstComponent>(info.id));
//      BFC_TEST_ASSERT_EQUAL(info.hasSecond, manager.has<SecondComponent>(info.id));
//      BFC_TEST_ASSERT_EQUAL(info.hasThird, manager.has<ThirdComponent>(info.id));
//    }
//  }
//
//  // Add components back and ensure they have no components
//  for (int64_t i = 0; i < 100; ++i) {
//    EntityID newID = manager.createEntity();
//    BFC_TEST_ASSERT_FALSE(manager.has<FirstComponent>(newID));
//    BFC_TEST_ASSERT_FALSE(manager.has<SecondComponent>(newID));
//    BFC_TEST_ASSERT_FALSE(manager.has<ThirdComponent>(newID));
//  }
//}
