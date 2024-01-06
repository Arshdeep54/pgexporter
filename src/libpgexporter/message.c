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
#include <logging.h>
#include <memory.h>
#include <message.h>
#include <network.h>
#include <utils.h>

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/time.h>

static int read_message(int socket, bool block, int timeout, message_t** msg);
static int write_message(int socket, message_t* msg);

static int ssl_read_message(SSL* ssl, int timeout, message_t** msg);
static int ssl_write_message(SSL* ssl, message_t* msg);

int
pgexporter_read_block_message(SSL* ssl, int socket, message_t** msg)
{
   if (ssl == NULL)
   {
      return read_message(socket, true, 0, msg);
   }

   return ssl_read_message(ssl, 0, msg);
}

int
pgexporter_read_timeout_message(SSL* ssl, int socket, int timeout, message_t** msg)
{
   if (ssl == NULL)
   {
      return read_message(socket, true, timeout, msg);
   }

   return ssl_read_message(ssl, timeout, msg);
}

int
pgexporter_write_message(SSL* ssl, int socket, message_t* msg)
{
   if (ssl == NULL)
   {
      return write_message(socket, msg);
   }

   return ssl_write_message(ssl, msg);
}

void
pgexporter_free_message(message_t* msg)
{
   pgexporter_memory_free();
}

message_t*
pgexporter_copy_message(message_t* msg)
{
   message_t* copy = NULL;

#ifdef DEBUG
   assert(msg != NULL);
   assert(msg->data != NULL);
   assert(msg->length > 0);
#endif

   copy = (message_t*)malloc(sizeof(message_t));
   copy->data = malloc(msg->length);

   copy->kind = msg->kind;
   copy->length = msg->length;
   memcpy(copy->data, msg->data, msg->length);

   return copy;
}

void
pgexporter_free_copy_message(message_t* msg)
{
   if (msg)
   {
      if (msg->data)
      {
         free(msg->data);
         msg->data = NULL;
      }

      free(msg);
      msg = NULL;
   }
}

bool
pgexporter_connection_isvalid(int socket)
{
   int status;
   int size = 15;

   char valid[size];
   message_t msg;
   message_t* reply = NULL;

   memset(&msg, 0, sizeof(message_t));
   memset(&valid, 0, sizeof(valid));

   pgexporter_write_byte(&valid, 'Q');
   pgexporter_write_int32(&(valid[1]), size - 1);
   pgexporter_write_string(&(valid[5]), "SELECT 1;");

   msg.kind = 'Q';
   msg.length = size;
   msg.data = &valid;

   status = write_message(socket, &msg);
   if (status != MESSAGE_STATUS_OK)
   {
      goto error;
   }

   status = read_message(socket, true, 0, &reply);
   if (status != MESSAGE_STATUS_OK)
   {
      goto error;
   }

   if (reply->kind == 'E')
   {
      goto error;
   }

   pgexporter_free_message(reply);

   return true;

error:
   if (reply)
   {
      pgexporter_free_message(reply);
   }

   return false;
}

void
pgexporter_log_message(message_t* msg)
{
   if (msg == NULL)
   {
      pgexporter_log_info("Message is NULL");
   }
   else if (msg->data == NULL)
   {
      pgexporter_log_info("Message DATA is NULL");
   }
   else
   {
      pgexporter_log_mem(msg->data, msg->length);
   }
}

int
pgexporter_write_empty(SSL* ssl, int socket)
{
   char zero[1];
   message_t msg;

   memset(&msg, 0, sizeof(message_t));
   memset(&zero, 0, sizeof(zero));

   msg.kind = 0;
   msg.length = 1;
   msg.data = &zero;

   if (ssl == NULL)
   {
      return write_message(socket, &msg);
   }

   return ssl_write_message(ssl, &msg);
}

int
pgexporter_write_notice(SSL* ssl, int socket)
{
   char notice[1];
   message_t msg;

   memset(&msg, 0, sizeof(message_t));
   memset(&notice, 0, sizeof(notice));

   notice[0] = 'N';

   msg.kind = 'N';
   msg.length = 1;
   msg.data = &notice;

   if (ssl == NULL)
   {
      return write_message(socket, &msg);
   }

   return ssl_write_message(ssl, &msg);
}

int
pgexporter_write_tls(SSL* ssl, int socket)
{
   char tls[1];
   message_t msg;

   memset(&msg, 0, sizeof(message_t));
   memset(&tls, 0, sizeof(tls));

   tls[0] = 'S';

   msg.kind = 'S';
   msg.length = 1;
   msg.data = &tls;

   if (ssl == NULL)
   {
      return write_message(socket, &msg);
   }

   return ssl_write_message(ssl, &msg);
}

int
pgexporter_write_terminate(SSL* ssl, int socket)
{
   char terminate[5];
   message_t msg;

   memset(&msg, 0, sizeof(message_t));
   memset(&terminate, 0, sizeof(terminate));

   pgexporter_write_byte(&terminate, 'X');
   pgexporter_write_int32(&(terminate[1]), 4);

   msg.kind = 'X';
   msg.length = 5;
   msg.data = &terminate;

   if (ssl == NULL)
   {
      return write_message(socket, &msg);
   }

   return ssl_write_message(ssl, &msg);
}

int
pgexporter_write_connection_refused(SSL* ssl, int socket)
{
   int size = 46;
   char connection_refused[size];
   message_t msg;

   memset(&msg, 0, sizeof(message_t));
   memset(&connection_refused, 0, sizeof(connection_refused));

   pgexporter_write_byte(&connection_refused, 'E');
   pgexporter_write_int32(&(connection_refused[1]), size - 1);
   pgexporter_write_string(&(connection_refused[5]), "SFATAL");
   pgexporter_write_string(&(connection_refused[12]), "VFATAL");
   pgexporter_write_string(&(connection_refused[19]), "C53300");
   pgexporter_write_string(&(connection_refused[26]), "Mconnection refused");

   msg.kind = 'E';
   msg.length = size;
   msg.data = &connection_refused;

   if (ssl == NULL)
   {
      return write_message(socket, &msg);
   }

   return ssl_write_message(ssl, &msg);
}

int
pgexporter_write_connection_refused_old(SSL* ssl, int socket)
{
   int size = 20;
   char connection_refused[size];
   message_t msg;

   memset(&msg, 0, sizeof(message_t));
   memset(&connection_refused, 0, sizeof(connection_refused));

   pgexporter_write_byte(&connection_refused, 'E');
   pgexporter_write_string(&(connection_refused[1]), "connection refused");

   msg.kind = 'E';
   msg.length = size;
   msg.data = &connection_refused;

   if (ssl == NULL)
   {
      return write_message(socket, &msg);
   }

   return ssl_write_message(ssl, &msg);
}

int
pgexporter_create_auth_password_response(char* password, message_t** msg)
{
   message_t* m = NULL;
   size_t size;

   size = 6 + strlen(password);

   m = (message_t*)malloc(sizeof(message_t));
   m->data = malloc(size);

   memset(m->data, 0, size);

   m->kind = 'p';
   m->length = size;

   pgexporter_write_byte(m->data, 'p');
   pgexporter_write_int32(m->data + 1, size - 1);
   pgexporter_write_string(m->data + 5, password);

   *msg = m;

   return MESSAGE_STATUS_OK;
}

int
pgexporter_create_auth_md5_response(char* md5, message_t** msg)
{
   message_t* m = NULL;
   size_t size;

   size = 1 + 4 + strlen(md5) + 1;

   m = (message_t*)malloc(sizeof(message_t));
   m->data = malloc(size);

   memset(m->data, 0, size);

   m->kind = 'p';
   m->length = size;

   pgexporter_write_byte(m->data, 'p');
   pgexporter_write_int32(m->data + 1, size - 1);
   pgexporter_write_string(m->data + 5, md5);

   *msg = m;

   return MESSAGE_STATUS_OK;
}

int
pgexporter_write_auth_scram256(SSL* ssl, int socket)
{
   char scram[24];
   message_t msg;

   memset(&msg, 0, sizeof(message_t));
   memset(&scram, 0, sizeof(scram));

   scram[0] = 'R';
   pgexporter_write_int32(&(scram[1]), 23);
   pgexporter_write_int32(&(scram[5]), 10);
   pgexporter_write_string(&(scram[9]), "SCRAM-SHA-256");

   msg.kind = 'R';
   msg.length = 24;
   msg.data = &scram;

   if (ssl == NULL)
   {
      return write_message(socket, &msg);
   }

   return ssl_write_message(ssl, &msg);
}

int
pgexporter_create_auth_scram256_response(char* nounce, message_t** msg)
{
   message_t* m = NULL;
   size_t size;

   size = 1 + 4 + 13 + 4 + 9 + strlen(nounce);

   m = (message_t*)malloc(sizeof(message_t));
   m->data = malloc(size);

   memset(m->data, 0, size);

   m->kind = 'p';
   m->length = size;

   pgexporter_write_byte(m->data, 'p');
   pgexporter_write_int32(m->data + 1, size - 1);
   pgexporter_write_string(m->data + 5, "SCRAM-SHA-256");
   pgexporter_write_string(m->data + 22, " n,,n=,r=");
   pgexporter_write_string(m->data + 31, nounce);

   *msg = m;

   return MESSAGE_STATUS_OK;
}

int
pgexporter_create_auth_scram256_continue(char* cn, char* sn, char* salt, message_t** msg)
{
   message_t* m = NULL;
   size_t size;

   size = 1 + 4 + 4 + 2 + strlen(cn) + strlen(sn) + 3 + strlen(salt) + 7;

   m = (message_t*)malloc(sizeof(message_t));
   m->data = malloc(size);

   memset(m->data, 0, size);

   m->kind = 'R';
   m->length = size;

   pgexporter_write_byte(m->data, 'R');
   pgexporter_write_int32(m->data + 1, size - 1);
   pgexporter_write_int32(m->data + 5, 11);
   pgexporter_write_string(m->data + 9, "r=");
   pgexporter_write_string(m->data + 11, cn);
   pgexporter_write_string(m->data + 11 + strlen(cn), sn);
   pgexporter_write_string(m->data + 11 + strlen(cn) + strlen(sn), ",s=");
   pgexporter_write_string(m->data + 11 + strlen(cn) + strlen(sn) + 3, salt);
   pgexporter_write_string(m->data + 11 + strlen(cn) + strlen(sn) + 3 + strlen(salt), ",i=4096");

   *msg = m;

   return MESSAGE_STATUS_OK;
}

int
pgexporter_create_auth_scram256_continue_response(char* wp, char* p, message_t** msg)
{
   message_t* m = NULL;
   size_t size;

   size = 1 + 4 + strlen(wp) + 3 + strlen(p);

   m = (message_t*)malloc(sizeof(message_t));
   m->data = malloc(size);

   memset(m->data, 0, size);

   m->kind = 'p';
   m->length = size;

   pgexporter_write_byte(m->data, 'p');
   pgexporter_write_int32(m->data + 1, size - 1);
   pgexporter_write_string(m->data + 5, wp);
   pgexporter_write_string(m->data + 5 + strlen(wp), ",p=");
   pgexporter_write_string(m->data + 5 + strlen(wp) + 3, p);

   *msg = m;

   return MESSAGE_STATUS_OK;
}

int
pgexporter_create_auth_scram256_final(char* ss, message_t** msg)
{
   message_t* m = NULL;
   size_t size;

   size = 1 + 4 + 4 + 2 + strlen(ss);

   m = (message_t*)malloc(sizeof(message_t));
   m->data = malloc(size);

   memset(m->data, 0, size);

   m->kind = 'R';
   m->length = size;

   pgexporter_write_byte(m->data, 'R');
   pgexporter_write_int32(m->data + 1, size - 1);
   pgexporter_write_int32(m->data + 5, 12);
   pgexporter_write_string(m->data + 9, "v=");
   pgexporter_write_string(m->data + 11, ss);

   *msg = m;

   return MESSAGE_STATUS_OK;
}

int
pgexporter_write_auth_success(SSL* ssl, int socket)
{
   char success[9];
   message_t msg;

   memset(&msg, 0, sizeof(message_t));
   memset(&success, 0, sizeof(success));

   success[0] = 'R';
   pgexporter_write_int32(&(success[1]), 8);
   pgexporter_write_int32(&(success[5]), 0);

   msg.kind = 'R';
   msg.length = 9;
   msg.data = &success;

   if (ssl == NULL)
   {
      return write_message(socket, &msg);
   }

   return ssl_write_message(ssl, &msg);
}

int
pgexporter_create_ssl_message(message_t** msg)
{
   message_t* m = NULL;
   size_t size;

   size = 8;

   m = (message_t*)malloc(sizeof(message_t));
   m->data = malloc(size);

   memset(m->data, 0, size);

   m->kind = 0;
   m->length = size;

   pgexporter_write_int32(m->data, size);
   pgexporter_write_int32(m->data + 4, 80877103);

   *msg = m;

   return MESSAGE_STATUS_OK;
}

int
pgexporter_create_startup_message(char* username, char* database, message_t** msg)
{
   message_t* m = NULL;
   size_t size;
   size_t us;
   size_t ds;

   us = strlen(username);
   ds = strlen(database);
   size = 4 + 4 + 4 + 1 + us + 1 + 8 + 1 + ds + 1 + 17 + 11 + 1;

   m = (message_t*)malloc(sizeof(message_t));
   m->data = malloc(size);

   memset(m->data, 0, size);

   m->kind = 0;
   m->length = size;

   pgexporter_write_int32(m->data, size);
   pgexporter_write_int32(m->data + 4, 196608);
   pgexporter_write_string(m->data + 8, "user");
   pgexporter_write_string(m->data + 13, username);
   pgexporter_write_string(m->data + 13 + us + 1, "database");
   pgexporter_write_string(m->data + 13 + us + 1 + 9, database);
   pgexporter_write_string(m->data + 13 + us + 1 + 9 + ds + 1, "application_name");
   pgexporter_write_string(m->data + 13 + us + 1 + 9 + ds + 1 + 17, "pgexporter");

   *msg = m;

   return MESSAGE_STATUS_OK;
}

static int
read_message(int socket, bool block, int timeout, message_t** msg)
{
   bool keep_read = false;
   ssize_t numbytes;
   struct timeval tv;
   message_t* m = NULL;

   if (unlikely(timeout > 0))
   {
      tv.tv_sec = timeout;
      tv.tv_usec = 0;
      setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
   }

   do
   {
      m = pgexporter_memory_message();

      numbytes = read(socket, m->data, m->max_length);

      if (likely(numbytes > 0))
      {
         m->kind = (signed char)(*((char*)m->data));
         m->length = numbytes;
         *msg = m;

         if (unlikely(timeout > 0))
         {
            tv.tv_sec = 0;
            tv.tv_usec = 0;
            setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
         }

         return MESSAGE_STATUS_OK;
      }
      else if (numbytes == 0)
      {
         pgexporter_memory_free();

         if ((errno == EAGAIN || errno == EWOULDBLOCK) && block)
         {
            keep_read = true;
            errno = 0;
         }
         else
         {
            if (unlikely(timeout > 0))
            {
               tv.tv_sec = 0;
               tv.tv_usec = 0;
               setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
            }

            return MESSAGE_STATUS_ZERO;
         }
      }
      else
      {
         pgexporter_memory_free();

         if ((errno == EAGAIN || errno == EWOULDBLOCK) && block)
         {
            keep_read = true;
            errno = 0;
         }
         else
         {
            keep_read = false;
         }
      }
   }
   while (keep_read);

   if (unlikely(timeout > 0))
   {
      tv.tv_sec = 0;
      tv.tv_usec = 0;
      setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

      pgexporter_memory_free();
   }

   return MESSAGE_STATUS_ERROR;
}

static int
write_message(int socket, message_t* msg)
{
   bool keep_write = false;
   ssize_t numbytes;
   int offset;
   ssize_t totalbytes;
   ssize_t remaining;

#ifdef DEBUG
   assert(msg != NULL);
#endif

   numbytes = 0;
   offset = 0;
   totalbytes = 0;
   remaining = msg->length;

   do
   {
      numbytes = write(socket, msg->data + offset, remaining);

      if (likely(numbytes == msg->length))
      {
         return MESSAGE_STATUS_OK;
      }
      else if (numbytes != -1)
      {
         offset += numbytes;
         totalbytes += numbytes;
         remaining -= numbytes;

         if (totalbytes == msg->length)
         {
            return MESSAGE_STATUS_OK;
         }

         pgexporter_log_debug("Write %d - %zd/%zd vs %zd", socket, numbytes, totalbytes, msg->length);
         keep_write = true;
         errno = 0;
      }
      else
      {
         switch (errno)
         {
            case EAGAIN:
               keep_write = true;
               errno = 0;
               break;
            default:
               keep_write = false;
               break;
         }
      }
   }
   while (keep_write);

   return MESSAGE_STATUS_ERROR;
}

static int
ssl_read_message(SSL* ssl, int timeout, message_t** msg)
{
   bool keep_read = false;
   ssize_t numbytes;
   time_t start_time;
   message_t* m = NULL;

   if (unlikely(timeout > 0))
   {
      start_time = time(NULL);
   }

   do
   {
      m = pgexporter_memory_message();

      numbytes = SSL_read(ssl, m->data, m->max_length);

      if (likely(numbytes > 0))
      {
         m->kind = (signed char)(*((char*)m->data));
         m->length = numbytes;
         *msg = m;

         return MESSAGE_STATUS_OK;
      }
      else
      {
         int err;

         pgexporter_memory_free();

         err = SSL_get_error(ssl, numbytes);
         switch (err)
         {
            case SSL_ERROR_ZERO_RETURN:
               if (timeout > 0)
               {
                  if (difftime(time(NULL), start_time) >= timeout)
                  {
                     return MESSAGE_STATUS_ZERO;
                  }

                  /* Sleep for 100ms */
                  SLEEP(100000000L);
               }
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_CONNECT:
            case SSL_ERROR_WANT_ACCEPT:
            case SSL_ERROR_WANT_X509_LOOKUP:
#ifndef HAVE_OPENBSD
#if (OPENSSL_VERSION_NUMBER >= 0x10100000L)
            case SSL_ERROR_WANT_ASYNC:
            case SSL_ERROR_WANT_ASYNC_JOB:
#if (OPENSSL_VERSION_NUMBER >= 0x10101000L)
            case SSL_ERROR_WANT_CLIENT_HELLO_CB:
#endif
#endif
#endif
               keep_read = true;
               break;
            case SSL_ERROR_SYSCALL:
               pgexporter_log_error("SSL_ERROR_SYSCALL: %s (%d)", strerror(errno), SSL_get_fd(ssl));
               errno = 0;
               keep_read = false;
               break;
            case SSL_ERROR_SSL:
               pgexporter_log_error("SSL_ERROR_SSL: %s (%d)", strerror(errno), SSL_get_fd(ssl));
               keep_read = false;
               break;
         }
         ERR_clear_error();
      }
   }
   while (keep_read);

   return MESSAGE_STATUS_ERROR;
}

static int
ssl_write_message(SSL* ssl, message_t* msg)
{
   bool keep_write = false;
   ssize_t numbytes;
   int offset;
   ssize_t totalbytes;
   ssize_t remaining;

#ifdef DEBUG
   assert(msg != NULL);
#endif

   numbytes = 0;
   offset = 0;
   totalbytes = 0;
   remaining = msg->length;

   do
   {
      numbytes = SSL_write(ssl, msg->data + offset, remaining);

      if (likely(numbytes == msg->length))
      {
         return MESSAGE_STATUS_OK;
      }
      else if (numbytes > 0)
      {
         offset += numbytes;
         totalbytes += numbytes;
         remaining -= numbytes;

         if (totalbytes == msg->length)
         {
            return MESSAGE_STATUS_OK;
         }

         pgexporter_log_debug("SSL/Write %d - %zd/%zd vs %zd", SSL_get_fd(ssl), numbytes, totalbytes, msg->length);
         keep_write = true;
         errno = 0;
      }
      else
      {
         int err = SSL_get_error(ssl, numbytes);

         switch (err)
         {
            case SSL_ERROR_ZERO_RETURN:
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_CONNECT:
            case SSL_ERROR_WANT_ACCEPT:
            case SSL_ERROR_WANT_X509_LOOKUP:
#ifndef HAVE_OPENBSD
#if (OPENSSL_VERSION_NUMBER >= 0x10100000L)
            case SSL_ERROR_WANT_ASYNC:
            case SSL_ERROR_WANT_ASYNC_JOB:
#if (OPENSSL_VERSION_NUMBER >= 0x10101000L)
            case SSL_ERROR_WANT_CLIENT_HELLO_CB:
#endif
#endif
#endif
               errno = 0;
               keep_write = true;
               break;
            case SSL_ERROR_SYSCALL:
               pgexporter_log_error("SSL_ERROR_SYSCALL: %s (%d)", strerror(errno), SSL_get_fd(ssl));
               errno = 0;
               keep_write = false;
               break;
            case SSL_ERROR_SSL:
               pgexporter_log_error("SSL_ERROR_SSL: %s (%d)", strerror(errno), SSL_get_fd(ssl));
               errno = 0;
               keep_write = false;
               break;
         }
         ERR_clear_error();

         if (!keep_write)
         {
            return MESSAGE_STATUS_ERROR;
         }
      }
   }
   while (keep_write);

   return MESSAGE_STATUS_ERROR;
}
