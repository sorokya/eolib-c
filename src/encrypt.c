#include "data.h"
#include "encrypt.h"

#if defined(_WIN32)
#include <windows.h>
#include <bcrypt.h>
static uint32_t csprng_uniform(uint32_t upper_bound)
{
    uint32_t val;
    BCryptGenRandom(NULL, (PUCHAR)&val, sizeof(val), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    return val % upper_bound;
}
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include <stdlib.h>
static uint32_t csprng_uniform(uint32_t upper_bound)
{
    return arc4random_uniform(upper_bound);
}
#else
#include <sys/random.h>
static uint32_t csprng_uniform(uint32_t upper_bound)
{
    uint32_t val;
    getrandom(&val, sizeof(val), 0);
    return val % upper_bound;
}
#endif

int32_t server_verification_hash(int32_t challenge)
{
    // See encrypt.h for formula reference.
    challenge++;
    return 110905 + ((challenge % 9) + 1) * ((11092004 - challenge) % (((challenge % 11) + 1) * 119)) * 119 + (challenge % 2004);
}

void swap_multiples(uint8_t *data, size_t length, uint8_t multiple)
{
    size_t sequence_length = 0;
    for (size_t i = 0; i < length; i++)
    {
        if (data[i] % multiple == 0)
        {
            sequence_length++;
        }
        else
        {
            if (sequence_length > 1)
            {
                size_t start = i - sequence_length;
                size_t end = start + sequence_length - 1;
                for (size_t j = 0; j < sequence_length / 2; j++)
                {
                    uint8_t temp = data[start + j];
                    data[start + j] = data[end - j];
                    data[end - j] = temp;
                }
            }

            sequence_length = 0;
        }
    }

    // Flush a trailing sequence at the end of the buffer
    if (sequence_length > 1)
    {
        size_t start = length - sequence_length;
        size_t end = start + sequence_length - 1;
        for (size_t j = 0; j < sequence_length / 2; j++)
        {
            uint8_t temp = data[start + j];
            data[start + j] = data[end - j];
            data[end - j] = temp;
        }
    }
}

uint8_t generate_swap_multiple()
{
    return (uint8_t)(csprng_uniform(7) + 6);
}

void encrypt_packet(uint8_t *data, size_t length, uint8_t swap_multiple)
{
    if (length < 2 || (data[0] == EO_BREAK_BYTE && data[1] == EO_BREAK_BYTE))
    {
        return;
    }

    swap_multiples(data, length, swap_multiple);

    // ceiling div
    size_t big_half = (length + 1) / 2;

    for (size_t i = 0; i < length; i++)
    {
        size_t next = (i < big_half) ? (i * 2) : ((length - 1 - i) * 2 + 1);
        if (next == i)
        {
            if ((data[i] & 0x7F) != 0)
            {
                data[i] ^= 0x80;
            }
            continue;
        }

        size_t j = next;
        while (j != i && j > i)
        {
            j = (j < big_half) ? (j * 2) : ((length - 1 - j) * 2 + 1);
        }
        if (j != i)
        {
            continue;
        }

        uint8_t temp = data[i];
        j = next;
        while (j != i)
        {
            uint8_t swap = data[j];
            if ((temp & 0x7F) != 0)
            {
                temp ^= 0x80;
            }
            data[j] = temp;
            temp = swap;
            j = (j < big_half) ? (j * 2) : ((length - 1 - j) * 2 + 1);
        }

        if ((temp & 0x7F) != 0)
        {
            temp ^= 0x80;
        }
        data[i] = temp;
    }
}

void decrypt_packet(uint8_t *data, size_t length, uint8_t swap_multiple)
{
    if (length < 2 || (data[0] == EO_BREAK_BYTE && data[1] == EO_BREAK_BYTE))
    {
        return;
    }

    for (size_t i = 0; i < length; i++)
    {
        size_t next = (i % 2 == 0) ? (i / 2) : (length - 1 - (i - 1) / 2);
        if (next == i)
        {
            if ((data[i] & 0x7F) != 0)
            {
                data[i] ^= 0x80;
            }
            continue;
        }

        size_t j = next;
        while (j != i && j > i)
        {
            j = (j % 2 == 0) ? (j / 2) : (length - 1 - (j - 1) / 2);
        }
        if (j != i)
        {
            continue;
        }

        uint8_t temp = data[i];
        j = next;
        while (j != i)
        {
            uint8_t swap = data[j];
            if ((temp & 0x7F) != 0)
            {
                temp ^= 0x80;
            }
            data[j] = temp;
            temp = swap;
            j = (j % 2 == 0) ? (j / 2) : (length - 1 - (j - 1) / 2);
        }

        if ((temp & 0x7F) != 0)
        {
            temp ^= 0x80;
        }
        data[i] = temp;
    }

    swap_multiples(data, length, swap_multiple);
}
