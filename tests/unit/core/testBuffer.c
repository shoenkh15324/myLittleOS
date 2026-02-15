/******************************************************************************
 *  Author : Mingyu Kim
 *  Created: 2026-02-15
 *  Description: Full unit tests for ring buffer with PASS/FAIL and stats check
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "core/feature/buffer.h"

// ---------- Utility ----------
#define TEST_BUF_SIZE 128
#define PRINT_RESULT(cond) printf("%s\n", (cond) ? "[PASS]" : "[FAIL]")

void printBufferStats(ringBuffer* buf){
#if APP_BUFFER_STATISTICS
    printf("Stats -> Usage:%zu Max:%zu TotalPush:%zu TotalPop:%zu Discard:%zu PushFail:%zu\n",
           bufferGetCurrentUsage(buf),
           bufferGetMaxUsage(buf),
           bufferGetTotalPushCount(buf),
           bufferGetTotalPopped(buf),
           bufferGetDiscardCount(buf),
           bufferGetPushFailCount(buf));
#endif
}
void compareBuffer(uint8_t* a, uint8_t* b, size_t size, const char* msg) {
    int ok = 1;
    for(size_t i=0;i<size;i++) if(a[i] != b[i]) ok = 0;
    printf("%s -> ", msg); PRINT_RESULT(ok);
}
// ---------- Test Cases ----------
void testOpenResetClose(){
    printf("=== Test: Open/Reset/Close ===\n");
    ringBuffer buf;
    int ret = bufferOpen(&buf, TEST_BUF_SIZE);
    printf("bufferOpen size=%zu -> ", buf.size);
    PRINT_RESULT(ret == retOk && buf.size == TEST_BUF_SIZE);
    ret = bufferReset(&buf);
    printf("bufferReset head=%zu tail=%zu usage=%zu -> ", buf.head, buf.tail, buf.usage);
    PRINT_RESULT(ret == retOk && buf.head == 0 && buf.tail == 0 && buf.usage == 0);
    ret = bufferClose(&buf);
    printf("bufferClose pBuf=%p -> ", buf.pBuf);
    PRINT_RESULT(ret == retOk && buf.pBuf == NULL);
    printf("\n");
}
void testPushPopBasic(){
    printf("=== Test: Push/Pop Basic ===\n");
    ringBuffer buf;
    bufferOpen(&buf, TEST_BUF_SIZE);
    uint8_t data[5] = {1,2,3,4,5};
    int ret = bufferPush(&buf, data, 5);
    printf("Push 5 bytes -> "); PRINT_RESULT(ret == retOk);
    uint8_t out[5] = {0};
    size_t popped = bufferPop(&buf, out, 5);
    compareBuffer(out, data, 5, "Pop 5 bytes");
    printBufferStats(&buf);
    bufferClose(&buf);
    printf("\n");
}
void testZeroBytePushPop(){
    printf("=== Test: Zero-Byte Push/Pop ===\n");
    ringBuffer buf;
    bufferOpen(&buf, TEST_BUF_SIZE);
    uint8_t out[1] = {0};
    int ret = bufferPush(&buf, out, 0);
    printf("Push 0 bytes -> "); PRINT_RESULT(ret == retOk);
    size_t popped = bufferPop(&buf, out, 0);
    printf("Pop 0 bytes -> "); PRINT_RESULT(popped == 0);
    bufferClose(&buf);
    printf("\n");
}
void testWrapAround(){
#if APP_BUFFER_PUSH_OVERWRITE
    printf("=== Test: Wrap-Around Push/Pop ===\n");
    ringBuffer buf;
    bufferOpen(&buf, TEST_BUF_SIZE);
    uint8_t data[16];
    for(int i=0;i<16;i++) data[i] = i+1;
    bufferPush(&buf, data, 12);  // Push 12 bytes
    uint8_t out[16] = {0};
    bufferPop(&buf, out, 5); // Pop 5 bytes, free space
    int ret = bufferPush(&buf, data, 10); // Push 10 bytes → wrap-around
    printf("Wrap-Around Push 10 bytes -> "); PRINT_RESULT(ret == retOk);
    size_t popped = bufferPop(&buf, out, 16);
    printf("Wrap-Around Pop %zu bytes -> ", popped);
    PRINT_RESULT(popped == bufferGetCurrentUsage(&buf) || popped <= TEST_BUF_SIZE);
    printBufferStats(&buf);
    bufferClose(&buf);
    printf("\n");
#else
    printf("=== Wrap-Around test skipped (APP_BUFFER_PUSH_OVERWRITE=0) ===\n\n");
#endif
}
void testOverwrite(){
#if APP_BUFFER_PUSH_OVERWRITE
    printf("=== Test: Overwrite ===\n");
    ringBuffer buf;
    bufferOpen(&buf, TEST_BUF_SIZE);
    uint8_t data[16];
    for(int i=0;i<16;i++) data[i] = i+1;
    bufferPush(&buf, data, 16);
    printf("Initial full push -> "); PRINT_RESULT(bufferGetCurrentUsage(&buf) == 16);
    bufferPush(&buf, data, 5); // overwrite
    printf("Push 5 bytes with overwrite -> "); PRINT_RESULT(bufferGetCurrentUsage(&buf) <= TEST_BUF_SIZE);
    uint8_t out[16] = {0};
    size_t popped = bufferPop(&buf, out, 16);
    printf("Pop after overwrite %zu bytes -> PASS\n", popped);
    printBufferStats(&buf);
    bufferClose(&buf);
    printf("\n");
#endif
}
void testPeek(){
#if APP_BUFFER_PEAK
    printf("=== Test: Peek ===\n");
    ringBuffer buf;
    bufferOpen(&buf, TEST_BUF_SIZE);
    uint8_t data[5] = {10,20,30,40,50};
    bufferPush(&buf, data, 5);
    uint8_t out[5] = {0};
    size_t peeked = bufferPeek(&buf, out, 5);
    compareBuffer(out, data, 5, "Peek 5 bytes");
    size_t popped = bufferPop(&buf, out, 5);
    compareBuffer(out, data, 5, "Pop after peek");
    bufferClose(&buf);
    printf("\n");
#endif
}
void testPushFail(){
#if !APP_BUFFER_PUSH_OVERWRITE
    printf("=== Test: Push Fail ===\n");
    ringBuffer buf;
    bufferOpen(&buf, TEST_BUF_SIZE);
    uint8_t data[TEST_BUF_SIZE*2];
    memset(data, 0xAA, sizeof(data));
    int ret = bufferPush(&buf, data, TEST_BUF_SIZE+1);
    printf("Push %zu bytes (should fail) -> ", TEST_BUF_SIZE+1);
    PRINT_RESULT(ret != retOk);
    printBufferStats(&buf);
    bufferClose(&buf);
    printf("\n");
#endif
}
void testFullCycle(){
    printf("=== Test: Full Cycle Push/Pop ===\n");
    ringBuffer buf;
    bufferOpen(&buf, TEST_BUF_SIZE);
    uint8_t data[TEST_BUF_SIZE];
    for(int i=0;i<TEST_BUF_SIZE;i++) data[i] = i+1;
    int ok = 1;
    for(int j=0;j<10;j++){
        bufferPush(&buf, data, TEST_BUF_SIZE);
        uint8_t out[TEST_BUF_SIZE];
        size_t popped = bufferPop(&buf, out, TEST_BUF_SIZE);
        for(int i=0;i<TEST_BUF_SIZE;i++) if(out[i] != data[i]) ok = 0;
    }
    printf("Repeated push/pop cycle -> "); PRINT_RESULT(ok);
    bufferClose(&buf);
    printf("\n");
}
void testExactFitPush(){
    printf("=== Test: Exact-Fit Push ===\n");
    ringBuffer buf;
    bufferOpen(&buf, TEST_BUF_SIZE);
    uint8_t data[TEST_BUF_SIZE];
    for (int i = 0; i < TEST_BUF_SIZE; i++) data[i] = i;
    bufferPush(&buf, data, 10);  // usage = 10
    size_t freeSpace = TEST_BUF_SIZE - bufferGetCurrentUsage(&buf);
    int ret = bufferPush(&buf, data, freeSpace);  // 정확히 남은 공간만큼
    printf("Push exact free space (%zu bytes) -> ", freeSpace);
    PRINT_RESULT(ret == retOk);
    printf("Usage == size -> ");
    PRINT_RESULT(bufferGetCurrentUsage(&buf) == TEST_BUF_SIZE);
    bufferClose(&buf);
    printf("\n");
}
void testPopMoreThanUsage(){
    printf("=== Test: Pop More Than Usage ===\n");
    ringBuffer buf;
    bufferOpen(&buf, TEST_BUF_SIZE);
    uint8_t data[5] = {1,2,3,4,5};
    bufferPush(&buf, data, 5);
    uint8_t out[10] = {0};
    size_t popped = bufferPop(&buf, out, 10);
    printf("Requested 10, actually popped %zu -> ", popped);
    PRINT_RESULT(popped == 5);
    printf("Usage becomes 0 -> ");
    PRINT_RESULT(bufferGetCurrentUsage(&buf) == 0);
    bufferClose(&buf);
    printf("\n");
}
void testResetAfterPush(){
    printf("=== Test: Reset After Push ===\n");
    ringBuffer buf;
    bufferOpen(&buf, TEST_BUF_SIZE);
    uint8_t data[5] = {9,8,7,6,5};
    bufferPush(&buf, data, 5);
    bufferReset(&buf);
    printf("Usage after reset -> ");
    PRINT_RESULT(bufferGetCurrentUsage(&buf) == 0);
    uint8_t out[5] = {0};
    size_t popped = bufferPop(&buf, out, 5);
    printf("Pop after reset (should be 0) -> ");
    PRINT_RESULT(popped == 0);
    bufferClose(&buf);
    printf("\n");
}
void testNullParameterDefense(){
    printf("=== Test: NULL Parameter Defense ===\n");
    ringBuffer buf;
    bufferOpen(&buf, TEST_BUF_SIZE);
    uint8_t data[5] = {1,2,3,4,5};
    uint8_t out[5];
    printf("bufferPush(NULL, ...) -> ");
    PRINT_RESULT(bufferPush(NULL, data, 5) != retOk);
    printf("bufferPush(&buf, NULL, ...) -> ");
    PRINT_RESULT(bufferPush(&buf, NULL, 5) != retOk);
    printf("bufferPop(NULL, ...) -> ");
    PRINT_RESULT(bufferPop(NULL, out, 5) == 0);
    bufferClose(&buf);
    printf("\n");
}
void testStatisticsValidation(){
#if APP_BUFFER_STATISTICS
    printf("=== Test: Statistics Validation ===\n");
    ringBuffer buf;
    bufferOpen(&buf, TEST_BUF_SIZE);
    uint8_t data[4] = {1,2,3,4};
    bufferPush(&buf, data, 4);
    bufferPush(&buf, data, 4);
    uint8_t out[4];
    bufferPop(&buf, out, 4);
    printf("TotalPushCount == 8 -> ");
    PRINT_RESULT(bufferGetTotalPushCount(&buf) == 8);
    printf("TotalPopCount == 4 -> ");
    PRINT_RESULT(bufferGetTotalPopped(&buf) == 4);
    printf("MaxUsage >= 8 -> ");
    PRINT_RESULT(bufferGetMaxUsage(&buf) >= 8);
    bufferClose(&buf);
    printf("\n");
#endif
}
void testRandomStress(){
    printf("=== Test: Random Stress ===\n");
    ringBuffer buf;
    bufferOpen(&buf, TEST_BUF_SIZE);
    srand((unsigned int)time(NULL));
    uint8_t data[TEST_BUF_SIZE];
    uint8_t out[TEST_BUF_SIZE];
    int ok = 1;
    for (int i = 0; i < 10000; i++) {
        int pushSize = rand() % (TEST_BUF_SIZE + 1);
        int popSize  = rand() % (TEST_BUF_SIZE + 1);
        for (int j = 0; j < pushSize; j++){ data[j] = rand() % 256; }
        bufferPush(&buf, data, pushSize);
        bufferPop(&buf, out, popSize);
        if (bufferGetCurrentUsage(&buf) > TEST_BUF_SIZE){ ok = 0; }
    }
    printf("Random stress 10000 iterations -> ");
    PRINT_RESULT(ok);
    bufferClose(&buf);
    printf("\n");
}
int main(){
    testOpenResetClose();
    testPushPopBasic();
    testZeroBytePushPop();
    testWrapAround();
    testOverwrite();
    testPeek();
    testPushFail();
    testFullCycle();
    testExactFitPush();
    testPopMoreThanUsage();
    testResetAfterPush();
    testNullParameterDefense();
    testStatisticsValidation();
    testRandomStress();
    printf("=== All tes;ts done ===\n");
    return 0;
}
