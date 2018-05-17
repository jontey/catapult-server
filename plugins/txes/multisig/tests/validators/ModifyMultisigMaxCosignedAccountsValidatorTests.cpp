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

#include "src/validators/Validators.h"
#include "tests/test/MultisigCacheTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS ModifyMultisigMaxCosignedAccountsValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(ModifyMultisigMaxCosignedAccounts, 0)

	namespace {
		void AssertValidationResult(
				ValidationResult expectedResult,
				uint8_t initialCosignedAccounts,
				uint8_t maxCosignedAccountsPerAccount) {
			// Arrange:
			auto multisigAccountKey = test::GenerateRandomData<Key_Size>();
			auto cosignatoryKey = test::GenerateRandomData<Key_Size>();

			// - setup cache
			auto cache = test::MultisigCacheFactory::Create();
			if (initialCosignedAccounts > 0) {
				auto cacheDelta = cache.createDelta();
				auto cosignatoryEntry = state::MultisigEntry(cosignatoryKey);

				// - add multisig accounts
				for (auto i = 0; i < initialCosignedAccounts; ++i)
					cosignatoryEntry.multisigAccounts().insert(test::GenerateRandomData<Key_Size>());

				cacheDelta.sub<cache::MultisigCache>().insert(cosignatoryEntry);
				cache.commit(Height());
			}

			// - create the validator context
			auto cacheView = cache.createView();
			auto readOnlyCache = cacheView.toReadOnly();
			auto context = test::CreateValidatorContext(Height(), readOnlyCache);

			model::ModifyMultisigNewCosignerNotification notification(multisigAccountKey, cosignatoryKey);
			auto pValidator = CreateModifyMultisigMaxCosignedAccountsValidator(maxCosignedAccountsPerAccount);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, context);

			// Assert:
			EXPECT_EQ(expectedResult, result)
					<< "initial " << static_cast<uint32_t>(initialCosignedAccounts)
					<< ", max " << static_cast<uint32_t>(maxCosignedAccountsPerAccount);
		}
	}

	TEST(TEST_CLASS, CanCosignFirstAccountWhenNoAccountsAreCosigned) {
		// Assert: notice that the multisig cache will not have an entry when no accounts are cosigned
		AssertValidationResult(ValidationResult::Success, 0, 10);
	}

	TEST(TEST_CLASS, CanCosignAdditionalAccountsWhenLessThanMaxAreCurrentlyCosigned) {
		// Assert:
		AssertValidationResult(ValidationResult::Success, 1, 10);
		AssertValidationResult(ValidationResult::Success, 9, 10);
	}

	TEST(TEST_CLASS, CannotCosignAdditionalAccountsWhenAtLeastMaxAreCurrentlyCosigned) {
		// Assert:
		AssertValidationResult(Failure_Multisig_Modify_Max_Cosigned_Accounts, 10, 10);
		AssertValidationResult(Failure_Multisig_Modify_Max_Cosigned_Accounts, 11, 10);
		AssertValidationResult(Failure_Multisig_Modify_Max_Cosigned_Accounts, 223, 10);
	}
}}