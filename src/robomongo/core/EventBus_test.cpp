#include <gtest/gtest.h>

#include <QObject>
#include <QPointer>

#include "robomongo-unit-tests/QtTestApplication.h"
#include "robomongo/core/Event.h"
#include "robomongo/core/EventBus.h"

using namespace Robomongo;

namespace
{
    int liveEvents = 0;
}

class BusTestEvent : public Event
{
    R_EVENT

public:
    BusTestEvent(QObject *sender, int value) : Event(sender), value(value) { ++liveEvents; }
    ~BusTestEvent() override { --liveEvents; }

    int const value;
};

class OtherBusTestEvent : public Event
{
    R_EVENT

public:
    explicit OtherBusTestEvent(QObject *sender) : Event(sender) {}
};

R_REGISTER_EVENT(BusTestEvent)
R_REGISTER_EVENT(OtherBusTestEvent)

class BusReceiver : public QObject
{
    Q_OBJECT

public:
    std::vector<int> values;
    std::vector<QObject *> senders;
    int otherCount = 0;

public Q_SLOTS:
    void handle(BusTestEvent *event)
    {
        values.push_back(event->value);
        senders.push_back(event->sender());
    }

    void handle(OtherBusTestEvent *) { ++otherCount; }
};

class EventBusTests : public ::testing::Test
{
protected:
    static void SetUpTestCase() { ensureQtTestApplication(); }
    void SetUp() override { liveEvents = 0; }

    EventBus bus;
    QObject senderA;
    QObject senderB;
};

TEST_F(EventBusTests, publishDeliversToSubscriber)
{
    BusReceiver receiver;
    bus.subscribe(&receiver, BusTestEvent::Type);

    bus.publish(new BusTestEvent(&senderA, 7));

    ASSERT_EQ(1u, receiver.values.size());
    EXPECT_EQ(7, receiver.values[0]);
    EXPECT_EQ(&senderA, receiver.senders[0]);
}

TEST_F(EventBusTests, publishDeliversToEverySubscriberInOrder)
{
    BusReceiver first;
    BusReceiver second;
    bus.subscribe(&first, BusTestEvent::Type);
    bus.subscribe(&second, BusTestEvent::Type);

    bus.publish(new BusTestEvent(&senderA, 1));
    bus.publish(new BusTestEvent(&senderA, 2));

    EXPECT_EQ(std::vector<int>({ 1, 2 }), first.values);
    EXPECT_EQ(std::vector<int>({ 1, 2 }), second.values);
}

TEST_F(EventBusTests, subscriptionsAreByEventType)
{
    BusReceiver receiver;
    bus.subscribe(&receiver, OtherBusTestEvent::Type);

    bus.publish(new OtherBusTestEvent(&senderA));
    bus.subscribe(&receiver, BusTestEvent::Type);
    bus.publish(new BusTestEvent(&senderA, 3));

    EXPECT_EQ(1, receiver.otherCount);
    EXPECT_EQ(std::vector<int>({ 3 }), receiver.values);
}

TEST_F(EventBusTests, senderFilterOnlyDeliversThatSendersEvents)
{
    BusReceiver receiver;
    bus.subscribe(&receiver, BusTestEvent::Type, &senderA);

    bus.publish(new BusTestEvent(&senderB, 1));
    bus.publish(new BusTestEvent(&senderA, 2));

    EXPECT_EQ(std::vector<int>({ 2 }), receiver.values);
}

TEST_F(EventBusTests, destroyedReceiversAreUnsubscribed)
{
    BusReceiver survivor;
    auto doomed = new BusReceiver();
    bus.subscribe(doomed, BusTestEvent::Type);
    bus.subscribe(&survivor, BusTestEvent::Type);

    delete doomed;
    bus.publish(new BusTestEvent(&senderA, 5));

    EXPECT_EQ(std::vector<int>({ 5 }), survivor.values);
}

TEST_F(EventBusTests, sendDeliversToOneReceiverWithoutSubscription)
{
    BusReceiver target;
    BusReceiver bystander;
    bus.subscribe(&bystander, BusTestEvent::Type);

    bus.send(&target, new BusTestEvent(&senderA, 9));

    EXPECT_EQ(std::vector<int>({ 9 }), target.values);
    EXPECT_TRUE(bystander.values.empty());
}

TEST_F(EventBusTests, sendToListDeliversToEachReceiver)
{
    BusReceiver first;
    BusReceiver second;

    bus.send(QList<QObject *>() << &first << &second, new BusTestEvent(&senderA, 4));

    EXPECT_EQ(std::vector<int>({ 4 }), first.values);
    EXPECT_EQ(std::vector<int>({ 4 }), second.values);
}

TEST_F(EventBusTests, deliveredEventsAreDeleted)
{
    BusReceiver receiver;
    bus.subscribe(&receiver, BusTestEvent::Type);

    bus.publish(new BusTestEvent(&senderA, 1));
    bus.send(&receiver, new BusTestEvent(&senderA, 2));

    EXPECT_EQ(0, liveEvents);
}

TEST_F(EventBusTests, eventsKnowTheirTypeAndErrorState)
{
    BusTestEvent ok(&senderA, 1);
    OtherBusTestEvent other(&senderA);

    EXPECT_EQ(BusTestEvent::Type, ok.type());
    EXPECT_NE(BusTestEvent::Type, other.type());
    EXPECT_STREQ("BusTestEvent*", ok.typeString());
    EXPECT_FALSE(ok.isError());
    EXPECT_EQ(&senderA, ok.sender());
}

TEST_F(EventBusTests, errorEventsCarryTheirError)
{
    class ErrorEvent : public Event
    {
    public:
        ErrorEvent(QObject *sender, const EventError &error) : Event(sender, error) {}
        QEvent::Type type() override { return QEvent::User; }
        const char *typeString() override { return "ErrorEvent*"; }
    };

    ErrorEvent const failed(&senderA, EventError("connection refused"));

    EXPECT_TRUE(failed.isError());
    // Messages are shown to users, so the first letter is capitalised
    EXPECT_EQ("Connection refused", failed.error().errorMessage());
}

#include "EventBus_test.moc"
