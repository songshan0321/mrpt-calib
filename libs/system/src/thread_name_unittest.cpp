/* +------------------------------------------------------------------------+
   |                     Mobile Robot Programming Toolkit (MRPT)            |
   |                          https://www.mrpt.org/                         |
   |                                                                        |
   | Copyright (c) 2005-2020, Individual contributors, see AUTHORS file     |
   | See: https://www.mrpt.org/Authors - All rights reserved.               |
   | Released under BSD License. See: https://www.mrpt.org/License          |
   +------------------------------------------------------------------------+ */

#include <gtest/gtest.h>
#include <mrpt/config.h>
#include <mrpt/core/format.h>
#include <mrpt/system/thread_name.h>

#include <atomic>
#include <condition_variable>

// This functionality is not implemented in OSX
#if !defined(MRPT_OS_APPLE)

TEST(thread_name, set_get_current_thread)
{
	const std::string testValue = "CoolName";
	const std::string testValue2 = "HotName";
	// Set:
	mrpt::system::thread_name(testValue);
	// Get:
	const std::string ret = mrpt::system::thread_name();
	EXPECT_EQ(ret, testValue);

	// Set:
	mrpt::system::thread_name(testValue2);
	// Get:
	const std::string ret2 = mrpt::system::thread_name();
	EXPECT_EQ(ret2, testValue2);
}

static std::condition_variable cv, cv2;
static std::mutex cv_m, cv2_m;
static std::atomic<int> thrCnt = 0;
static std::atomic<int> thrDone = 0;

static const std::string sNewName = "NewName";

static void testerThread(const std::string& myName)
{
	thrCnt++;

	// 1: wait for synchro point.
	{
		std::unique_lock<std::mutex> lk(cv_m);
		cv2.notify_one();
		cv.wait(lk);
	}

	// 2: check name matches:
	EXPECT_EQ(myName, mrpt::system::thread_name());

	// 3: change name:
	mrpt::system::thread_name(sNewName);

	// 4: signal:
	thrDone++;
	cv2.notify_one();

	// 5: wait for final signal
	{
		std::unique_lock<std::mutex> lk(cv_m);
		cv.wait(lk);
	}
}

TEST(thread_name, set_get_other_thread)
{
	constexpr unsigned int N = 10;
	std::vector<std::thread> threads;
	std::vector<std::string> names;

	for (unsigned int i = 0; i < N; i++)
	{
		const std::string thName = mrpt::format("tester_%u", i);
		std::thread& t = threads.emplace_back(&testerThread, thName);
		names.push_back(thName);

		mrpt::system::thread_name(thName, t);
	}
	{
		std::unique_lock<std::mutex> lk(cv2_m);
		cv2.wait(lk, [&] { return thrCnt == N; });
	}
	cv.notify_all();

	{
		std::unique_lock<std::mutex> lk(cv2_m);
		cv2.wait(lk, [&] { return thrDone == N; });
	}

	for (auto& t : threads)
	{
		EXPECT_EQ(mrpt::system::thread_name(t), sNewName);
	}

	cv.notify_all();

	for (auto& t : threads)
		if (t.joinable()) t.join();
}

#endif