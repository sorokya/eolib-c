#include "encrypt.h"

int32_t server_verification_hash(int32_t challenge)
{
    challenge++;
    return 110905 + ((challenge % 9) + 1) * ((11092004 - challenge) % (((challenge % 11) + 1) * 119)) * 119 + (challenge % 2004);
}

void swap_multiples(uint8_t *data, size_t length, uint8_t multiple)
{
    size_t sequence_length = 0;
    for (size_t i = 0; i <= length; i++)
    {
        if (i != length && data[i] % multiple == 0)
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

                sequence_length = 0;
            }
        }
    }
}

uint8_t generate_swap_multiple()
{
    return (rand() % 7) + 6;
}
