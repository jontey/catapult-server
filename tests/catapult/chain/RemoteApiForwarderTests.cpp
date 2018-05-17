/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "catapult/chain/RemoteApiForwarder.h"
#include "tests/test/core/mocks/MockPacketIo.h"
#include "tests/test/net/mocks/MockPacketWriters.h"
#include "tests/TestHarness.h"

namespace catapult { namespace chain {

#define TEST_CLASS RemoteApiForwarderTests

	namespace {
		constexpr int Default_Action_Api_Id = 7;

		struct ProcessSyncParamsCapture {
			size_t NumFactoryCalls = 0;
			const ionet::PacketIo* pFactoryPacketIo = nullptr;
			const model::TransactionRegistry* pFactoryTransactionRegistry = nullptr;

			size_t NumActionCalls = 0;
			int ActionApiId = 0;
		};

		thread::future<NodeInteractionResult> ProcessSyncAndCapture(RemoteApiForwarder& forwarder, ProcessSyncParamsCapture& capture) {
			return forwarder.processSync(
				[&capture](const auto& apiId) {
					++capture.NumActionCalls;
					capture.ActionApiId = apiId;
					return thread::make_ready_future(NodeInteractionResult::Success);
				},
				[&capture](const auto& packetIo, const auto& registry) {
					++capture.NumFactoryCalls;
					capture.pFactoryPacketIo = &packetIo;
					capture.pFactoryTransactionRegistry = &registry;
					return std::make_unique<int>(Default_Action_Api_Id);
				});
		}
	}

	TEST(TEST_CLASS, ActionIsSkippedWhenNoPeerIsAvailable) {
		// Arrange: create an empty writers
		mocks::PickOneAwareMockPacketWriters writers;

		// - create the forwarder
		model::TransactionRegistry registry;
		RemoteApiForwarder forwarder(writers, registry, utils::TimeSpan::FromSeconds(4), "test");

		// Act:
		ProcessSyncParamsCapture capture;
		auto result = ProcessSyncAndCapture(forwarder, capture).get();

		// Assert:
		EXPECT_EQ(NodeInteractionResult::None, result);

		// - pick one was called
		ASSERT_EQ(1u, writers.numPickOneCalls());
		EXPECT_EQ(utils::TimeSpan::FromSeconds(4), writers.pickOneDurations()[0]);

		// - other calls were bypassed
		EXPECT_EQ(0u, capture.NumFactoryCalls);
		EXPECT_EQ(0u, capture.NumActionCalls);
	}

	TEST(TEST_CLASS, ActionIsInvokedWhenPeerIsAvailable) {
		// Arrange: create writers with a valid packet
		auto pPacketIo = std::make_shared<mocks::MockPacketIo>();
		mocks::PickOneAwareMockPacketWriters writers;
		writers.setPacketIo(pPacketIo);

		// - create the forwarder
		model::TransactionRegistry registry;
		RemoteApiForwarder forwarder(writers, registry, utils::TimeSpan::FromSeconds(4), "test");

		// Act:
		ProcessSyncParamsCapture capture;
		auto result = ProcessSyncAndCapture(forwarder, capture).get();

		// Assert:
		EXPECT_EQ(NodeInteractionResult::Success, result);

		// - pick one was called
		ASSERT_EQ(1u, writers.numPickOneCalls());
		EXPECT_EQ(utils::TimeSpan::FromSeconds(4), writers.pickOneDurations()[0]);

		// - factory was called
		EXPECT_EQ(1u, capture.NumFactoryCalls);
		EXPECT_EQ(pPacketIo.get(), capture.pFactoryPacketIo);
		EXPECT_EQ(&registry, capture.pFactoryTransactionRegistry);

		// - action was called
		EXPECT_EQ(1u, capture.NumActionCalls);
		EXPECT_EQ(Default_Action_Api_Id, capture.ActionApiId);
	}
}}