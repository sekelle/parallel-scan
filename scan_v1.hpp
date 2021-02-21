/*
 * MIT License
 *
 * Copyright (c) 2021 Sebastian Keller
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*! \file
 * \brief Parallel prefix sum
 *
 * \author Sebastian Keller <sebastian.f.keller@gmail.com>
 */

#pragma once

#include <algorithm>
#include <iostream>

#include <omp.h>

namespace v1
{

template<class T>
void exclusiveScan(const T* in, T* out, size_t numElements)
{
    constexpr int blockSize = (8192 + 16384) / sizeof(T);
    //constexpr int blockSize = 4;

    int numThreads = 1;
    #pragma omp parallel
    {
        #pragma omp single
        numThreads = omp_get_num_threads();
    }

    T superBlock[2][numThreads+1];
    std::fill(superBlock[0], superBlock[0] + numThreads+1, 0);
    std::fill(superBlock[1], superBlock[1] + numThreads+1, 0);

    unsigned elementsPerStep = numThreads * blockSize;
    unsigned nSteps = numElements / elementsPerStep;

    #pragma omp parallel num_threads(numThreads)
    {
        int tid = omp_get_thread_num();
        for (size_t step = 0; step < nSteps; ++step)
        {
            size_t stepOffset = step * elementsPerStep + tid * blockSize;

            T tSum = 0;
            for (size_t ib = 0; ib < blockSize; ++ib)
            {
                size_t i = stepOffset + ib;

                out[i] = tSum;
                tSum += in[i];
            }

            superBlock[step%2][tid] = tSum;

            #pragma omp barrier

            tSum = superBlock[(step+1)%2][numThreads];
            for (size_t t = 0; t < tid; ++t)
                tSum += superBlock[step%2][t];

            if (tid == numThreads - 1)
            {
                superBlock[step%2][numThreads] = tSum + superBlock[step%2][numThreads - 1];
            }

            for (size_t ib = 0; ib < blockSize; ++ib)
            {
                size_t i = stepOffset + ib;
                out[i] += tSum;
            }
        }
    }

    T stepSum = superBlock[(nSteps+1)%2][numThreads];
    for (size_t i = nSteps * elementsPerStep; i < numElements; ++i)
    {
        out[i] = stepSum;
        stepSum += in[i];
    }

}

} // namespace v1
