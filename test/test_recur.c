/*
 * Copyright (c) 2025 windflower
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>

#include "coheader.h"
#include "coroutine.h"
int recnum = 0;
void *auto_rec(const void *) {
    if (recnum >= 5) return NULL;
    char name[2];
    name[0] = 'a';
    name[1] = '\0';
    name[0] += recnum;
    recnum++;
    printf("co %s begin\n", name);
    coroutine_t co1, co2;
    co1 = coroutine_create(auto_rec, NULL, 0);
    co2 = coroutine_create(auto_rec, NULL, 0);
    coroutine_join(co1);
    coroutine_join(co2);
    printf("co %s finished\n", name);
    return NULL;
}
int main() {
    printf("***************test auto rec**************\n");
    auto_rec(NULL);
}
/*正确输出：
***************test auto rec**************
co a begin
co b begin
co c begin
co d begin
co e begin
co c finished
co d finished
co e finished
co b finished
co a finished

*/