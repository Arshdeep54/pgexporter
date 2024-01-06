/*
 * Copyright (C) 2024 The pgexporter community
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list
 * of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or other
 * materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may
 * be used to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* pgexporter */
#include <pgexporter.h>
#include <memory.h>
#include <message.h>

/* system */
#ifdef DEBUG
#include <assert.h>
#endif
#include <stdlib.h>
#include <string.h>

static message_t* message = NULL;
static void* data = NULL;

/**
 *
 */
void
pgexporter_memory_init(void)
{
   configuration_t* config;

   config = (configuration_t*)shmem;

   pgexporter_memory_size(config->buffer_size);
}

/**
 *
 */
void
pgexporter_memory_size(size_t size)
{
   pgexporter_memory_destroy();

   message = (message_t*)malloc(sizeof(message_t));
   data = malloc(size);

   memset(message, 0, sizeof(message_t));
   memset(data, 0, size);

   message->kind = 0;
   message->length = 0;
   message->max_length = size;
   message->data = data;
}

/**
 *
 */
message_t*
pgexporter_memory_message(void)
{
#ifdef DEBUG
   assert(message != NULL);
   assert(data != NULL);
#endif

   return message;
}

/**
 *
 */
void
pgexporter_memory_free(void)
{
   size_t length = message->max_length;

#ifdef DEBUG
   assert(message != NULL);
   assert(data != NULL);
#endif

   memset(message, 0, sizeof(message_t));
   memset(data, 0, length);

   message->kind = 0;
   message->length = 0;
   message->max_length = length;
   message->data = data;
}

/**
 *
 */
void
pgexporter_memory_destroy(void)
{
   if (data != NULL)
   {
      free(data);
   }
   if (message != NULL)
   {
      free(message);
   }

   data = NULL;
   message = NULL;
}
