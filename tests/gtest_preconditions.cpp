#include <gtest/gtest.h>
#include <string>
#include "behaviortree_cpp/basic_types.h"
#include "behaviortree_cpp/bt_factory.h"
#include "test_helper.hpp"

using namespace BT;

TEST(PreconditionsDecorator, Integers)
{
  BehaviorTreeFactory factory;
  std::array<int, 3> counters;
  RegisterTestTick(factory, "Test", counters);

  const std::string xml_text = R"(

    <root BTCPP_format="4" >
        <BehaviorTree ID="MainTree">
            <Sequence>
                <Script code = "A:=1; B:=1; C:=3" />
                <Precondition if="A==B" else="FAILURE">
                    <TestA/>
                </Precondition>
                <Precondition if="A==C" else="SUCCESS">
                    <TestB/>
                </Precondition>
                <Precondition if="A!=C" else="FAILURE">
                    <TestC/>
                </Precondition>
            </Sequence>
        </BehaviorTree>
    </root>)";

  auto tree = factory.createTreeFromText(xml_text);
  const auto status = tree.tickWhileRunning();
  ASSERT_EQ(status, NodeStatus::SUCCESS);
  ASSERT_EQ(counters[0], 1);
  ASSERT_EQ(counters[1], 0);
  ASSERT_EQ(counters[2], 1);
}

TEST(PreconditionsDecorator, DoubleEquals)
{
  BehaviorTreeFactory factory;
  std::array<int, 3> counters;
  RegisterTestTick(factory, "Test", counters);

  const std::string xml_text = R"(

    <root BTCPP_format="4" >
        <BehaviorTree ID="MainTree">
            <Sequence>
                <Script code = " A:=1.1; B:=(1.0+0.1); C:= 2.0 " />

                <Precondition if="A==B" else="FAILURE">
                    <TestA/>
                </Precondition>

                <Precondition if="A==C" else="SUCCESS">
                    <TestB/>
                </Precondition>

                <Precondition if="A!=C" else="FAILURE">
                    <TestC/>
                </Precondition>
            </Sequence>
        </BehaviorTree>
    </root>)";

  auto tree = factory.createTreeFromText(xml_text);
  const auto status = tree.tickWhileRunning();
  ASSERT_EQ(status, NodeStatus::SUCCESS);
  ASSERT_EQ(counters[0], 1);
  ASSERT_EQ(counters[1], 0);
  ASSERT_EQ(counters[2], 1);
}

TEST(PreconditionsDecorator, StringEquals)
{
  BehaviorTreeFactory factory;
  std::array<int, 2> counters;
  RegisterTestTick(factory, "Test", counters);

  const std::string xml_text = R"(

    <root BTCPP_format="4" >
        <BehaviorTree ID="MainTree">
            <Sequence>
                <Script code = "A:='hello'" />
                <Script code = "B:='world'" />
                <Script code = "C:='world'" />

                <Precondition if=" A==B " else="SUCCESS">
                    <TestA/>
                </Precondition>
                <Precondition if=" B==C " else="FAILURE">
                    <TestB/>
                </Precondition>
            </Sequence>
        </BehaviorTree>
    </root>)";

  auto tree = factory.createTreeFromText(xml_text);
  const auto status = tree.tickWhileRunning();
  ASSERT_EQ(status, NodeStatus::SUCCESS);
  ASSERT_EQ(counters[0], 0);
  ASSERT_EQ(counters[1], 1);
}

TEST(Preconditions, Basic)
{
  BehaviorTreeFactory factory;
  std::array<int, 4> counters;
  RegisterTestTick(factory, "Test", counters);

  const std::string xml_text = R"(

    <root BTCPP_format="4" >
        <BehaviorTree ID="MainTree">
            <Sequence>
                <Script code = "A:=1" />
                <TestA _successIf= "A==1"/>
                <TestB _successIf= "A==2"/>
                <Fallback>
                    <TestC _failureIf= "A==1"/>
                    <TestD _failureIf= "A!=1"/>
                </Fallback>
            </Sequence>
        </BehaviorTree>
    </root>)";

  auto tree = factory.createTreeFromText(xml_text);
  const auto status = tree.tickWhileRunning();
  ASSERT_EQ(status, NodeStatus::SUCCESS);
  ASSERT_EQ(counters[0], 0);   // skipped
  ASSERT_EQ(counters[1], 1);   // executed
  ASSERT_EQ(counters[2], 0);   // skipped
  ASSERT_EQ(counters[3], 1);   // executed
}


TEST(Preconditions, Issue533)
{
  BehaviorTreeFactory factory;
  std::array<int, 3> counters;
  RegisterTestTick(factory, "Test", counters);

  const std::string xml_text = R"(

    <root BTCPP_format="4" >
        <BehaviorTree ID="MainTree">
            <Sequence>
                <TestA _skipIf="A!=1" />
                <TestB _skipIf="A!=2" _onSuccess="A=1"/>
                <TestC _skipIf="A!=3" _onSuccess="A=2"/>
            </Sequence>
        </BehaviorTree>
    </root>)";

  auto tree = factory.createTreeFromText(xml_text);
  tree.subtrees.front()->blackboard->set("A", 3);

  tree.tickOnce();
  ASSERT_EQ(counters[0], 0);
  ASSERT_EQ(counters[1], 0);
  ASSERT_EQ(counters[2], 1);

  tree.tickOnce();
  ASSERT_EQ(counters[0], 0);
  ASSERT_EQ(counters[1], 1);
  ASSERT_EQ(counters[2], 1);

  tree.tickOnce();
  ASSERT_EQ(counters[0], 1);
  ASSERT_EQ(counters[1], 1);
  ASSERT_EQ(counters[2], 1);
}


class CoroTestNode: public BT::CoroActionNode
{
public:
  CoroTestNode(const std::string& node_name,
               const BT::NodeConfig& config) :
    BT::CoroActionNode(node_name, config)
  {}

  virtual BT::NodeStatus tick() override
  {
    for(int i=0; i<10; i++) {
      times_ticked++;
      setStatusRunningAndYield();
    }
    return NodeStatus::SUCCESS;
  }

  static PortsList providedPorts()
  {
    return {};
  }

  int times_ticked = 0;
};


TEST(Preconditions, Issue585)
{
  BehaviorTreeFactory factory;
  factory.registerNodeType<CoroTestNode>("CoroTest");

  const std::string xml_text = R"(

    <root BTCPP_format="4" >
        <BehaviorTree ID="MainTree">
            <Sequence>
                <Script    code="A:=1" />
                <CoroTest _skipIf="A==1" />
            </Sequence>
        </BehaviorTree>
    </root>)";

  auto tree = factory.createTreeFromText(xml_text);
  tree.tickWhileRunning();

  auto coro = dynamic_cast<CoroTestNode*>(tree.subtrees.front()->nodes.back().get());
  ASSERT_EQ(coro->times_ticked, 0);
}

TEST(Preconditions, Issue615_NoSkipWhenRunning_A)
{
  static constexpr auto xml_text = R"(
  <root BTCPP_format="4">
  <BehaviorTree>
    <KeepRunningUntilFailure _skipIf="check == true">
      <AlwaysSuccess/>
    </KeepRunningUntilFailure>
  </BehaviorTree>
  </root> )";

  BehaviorTreeFactory factory;
  auto tree = factory.createTreeFromText(xml_text);

  tree.rootBlackboard()->set("check", false);
  ASSERT_EQ( tree.tickOnce(), NodeStatus::RUNNING );

  // the precondition should NOT be called, because
  // KeepRunningUntilFailure is in RUNNING state
  tree.rootBlackboard()->set("check", true);
  ASSERT_EQ( tree.tickOnce(), NodeStatus::RUNNING );
}

class KeepRunning : public BT::StatefulActionNode
{
public:
  KeepRunning(const std::string& name, const BT::NodeConfig& config) :
    BT::StatefulActionNode(name, config){}

  static BT::PortsList providedPorts() {
    return {};
  }

  BT::NodeStatus onStart() override {
    return BT::NodeStatus::RUNNING;
  }

  BT::NodeStatus onRunning() override
  {
    return BT::NodeStatus::RUNNING;
  }

  void onHalted() override {
    std::cout << "Node halted\n";
  }
};

TEST(Preconditions, Issue615_NoSkipWhenRunning_B)
{
  static constexpr auto xml_text = R"(
  <root BTCPP_format="4">
  <BehaviorTree>
    <KeepRunning _skipIf="check==false"/>
  </BehaviorTree>
  </root>
  )";

  BehaviorTreeFactory factory;
  factory.registerNodeType<KeepRunning>("KeepRunning");
  auto tree = factory.createTreeFromText(xml_text);

  tree.rootBlackboard()->set("check", false);
  ASSERT_EQ( tree.tickOnce(), NodeStatus::SKIPPED );

  // Should not be skipped anymore
  tree.rootBlackboard()->set("check", true);
  ASSERT_EQ( tree.tickOnce(), NodeStatus::RUNNING );

  // skipIf should be ignored, because KeepRunning is RUNNING and not IDLE
  tree.rootBlackboard()->set("check", false);
  ASSERT_EQ( tree.tickOnce(), NodeStatus::RUNNING );
}



TEST(Preconditions, Remapping)
{
  static constexpr auto xml_text = R"(
  <root BTCPP_format="4">

    <BehaviorTree ID="Main">
      <Sequence>
        <Script  code="value:=1" />
        <SubTree ID="Sub1" param="{value}"/>
        <TestA _skipIf="value!=1" />
      </Sequence>
    </BehaviorTree>

    <BehaviorTree ID="Sub1">
      <Sequence>
        <TestB _skipIf="param!=1" />
      </Sequence>
    </BehaviorTree>
  </root>
  )";

  BehaviorTreeFactory factory;

  std::array<int, 2> counters;
  RegisterTestTick(factory, "Test", counters);

  factory.registerBehaviorTreeFromText(xml_text);
  auto tree = factory.createTree("Main");

  auto status = tree.tickWhileRunning();

  ASSERT_EQ(status, BT::NodeStatus::SUCCESS);
  ASSERT_EQ( counters[0], 1 );
  ASSERT_EQ( counters[1], 1 );
}
