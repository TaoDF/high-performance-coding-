typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

#define BLOCK_BITS      512
#define BLOCK_BYTES     (BLOCK_BITS / 8)
#define HASH_BITS       256
#define HASH_BYTES      (HASH_BITS / 8)
#define SECRET_BYTES    16
#define SECRET_WORDS    (SECRET_BYTES / 4)
#define BYTES_PER_WORD  4

// #define ENABLE_PRINT

//-----------------------------------------------------------------------------
// SHA256
//-----------------------------------------------------------------------------

#define ZERO_ARRAY(array, n) \
    for (int counter = 0; counter < n; counter++) { \
        array[counter] = 0; \
    }

inline uint32_t rightrotate(uint32_t x, int n) {
    return ((x >> n) | (x << (32 - n)));
}

inline void writeIntToCharArray(uint8_t* buffer, int i, uint32_t fourByte) {
    buffer[i + 0] = (fourByte >> 24) & 0xFF;
    buffer[i + 1] = (fourByte >> 16) & 0xFF;
    buffer[i + 2] = (fourByte >>  8) & 0xFF;
    buffer[i + 3] = (fourByte >>  0) & 0xFF;
}

inline uint32_t expandCharArrayToInt(uint8_t* buffer, int i) {
    return (buffer[i] << 24) | (buffer[i + 1] << 16) | (buffer[i + 2] << 8) | (buffer[i + 3]);
}

// !!! Does not work for all binary input data !!!
// Assume input is 8-bit aligned (e.g. char)
// Assume input is at most 2^32-1 BITS long (i.e. length can fit in 32-bit int)
// Output is 256-bits (32 bytes)
void SHA256(const uint8_t* input, 
            const int inputLen,
            uint8_t* output)
{
    uint32_t k[64]={
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    uint32_t h0 = 0x6a09e667;
    uint32_t h1 = 0xbb67ae85;
    uint32_t h2 = 0x3c6ef372;
    uint32_t h3 = 0xa54ff53a;
    uint32_t h4 = 0x510e527f;
    uint32_t h5 = 0x9b05688c;
    uint32_t h6 = 0x1f83d9ab;
    uint32_t h7 = 0x5be0cd19;

    int L = inputLen * 8;
    int K = 0;
    int chunks = (L + 1 + K + 64) / 512; // Each chunk is 512-bits / 64 bytes / 16 ints

    // Check if we need extra padding to fit everything inside 512-bit blocks
    if (L + 1 + 64 > chunks * 512) {
        chunks += 1;
        K = (chunks * 512) - L - 1 - 64;
    }

    #ifdef ENABLE_PRINT
    printf("SHA256 inputLen:%d L:%d chunks:%d K:%d\n", inputLen, L, chunks, K);
    #endif

    uint32_t w[64];
    bool reachedEnd = false;
    for (int chunk = 0; chunk < chunks; chunk++) {
        ZERO_ARRAY(w, 64);

        // Copy chunk into first 16 elements of w
        // 1 chunk = 512 bits = 64 bytes/chars = 16 uint32_t
        for (int i = 0; i < 16; i++) {
            int offset;
            int idx;

            // We unrolled the following 4 repetitive steps because
            // we don't want to use goto to break out of the i-loop

            offset = 24;
            idx = (chunk * 64) + (i * 4) + 0;
            if (idx >= inputLen) {
                w[i] |= (0x80 << offset); // We assume input is 8-bit aligned so the final '1' pad is always 0x80
                reachedEnd = true;
                break;
            }
            w[i] |= input[idx] << offset;

            offset = 16;
            idx = (chunk * 64) + (i * 4) + 1;
            if (idx >= inputLen) {
                w[i] |= (0x80 << offset);
                reachedEnd = true;
                break;
            }
            w[i] |= input[idx] << offset;

            offset = 8;
            idx = (chunk * 64) + (i * 4) + 2;
            if (idx >= inputLen) {
                w[i] |= (0x80 << offset);
                reachedEnd = true;
                break;
            }
            w[i] |= input[idx] << offset;

            offset = 0;
            idx = (chunk * 64) + (i * 4) + 3;
            if (idx >= inputLen) {
                w[i] |= (0x80 << offset);
                reachedEnd = true;
                break;
            }
            w[i] |= input[idx] << offset;
        }

        if (reachedEnd) {
            // SHA256 spec requires us to append a 64-bit int L to specify bitsize of message
            // Last two bytes of w therefore must be the upper/lower half of L
            // Since we assume message length is 32bit, we only need the lower half in w[15]
            w[15] = L;
        }

        #ifdef ENABLE_PRINT
        printf("w:\n");
        for (int i = 0; i < 64; i++) {
            printf("%02d %#010x\n", i, w[i]);
        }
        #endif

        // Here's where the magic starts
        // Better hope there's no typo

        for (int i = 16; i < 64; i++) {
            uint32_t s0 = rightrotate(w[i - 15],  7) ^ rightrotate(w[i - 15], 18) ^ (w[i - 15] >>  3);
            uint32_t s1 = rightrotate(w[i -  2], 17) ^ rightrotate(w[i -  2], 19) ^ (w[i -  2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        uint32_t a = h0;
        uint32_t b = h1;
        uint32_t c = h2;
        uint32_t d = h3;
        uint32_t e = h4;
        uint32_t f = h5;
        uint32_t g = h6;
        uint32_t h = h7;

        for (int i = 0; i < 64; i++) {
            uint32_t S1    = rightrotate(e, 6) ^ rightrotate(e, 11) ^ rightrotate(e, 25);
            uint32_t ch    = (e & f) ^ (~e & g);
            uint32_t temp1 = h + S1 + ch + k[i] + w[i];
            uint32_t S0    = rightrotate(a, 2) ^ rightrotate(a, 13) ^ rightrotate(a, 22);
            uint32_t maj   = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = S0 + maj;
     
            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
        h5 += f;
        h6 += g;
        h7 += h;
    }

    writeIntToCharArray(output, 0 * 4, h0);
    writeIntToCharArray(output, 1 * 4, h1);
    writeIntToCharArray(output, 2 * 4, h2);
    writeIntToCharArray(output, 3 * 4, h3);
    writeIntToCharArray(output, 4 * 4, h4);
    writeIntToCharArray(output, 5 * 4, h5);
    writeIntToCharArray(output, 6 * 4, h6);
    writeIntToCharArray(output, 7 * 4, h7);
}

//-----------------------------------------------------------------------------
// HMAC
//-----------------------------------------------------------------------------

// messageBuffer must be at least messsageLen + BLOCK_BYTES big
void HMAC_SHA256(const uint8_t* secret,
                 const int secretLen,
                 const uint8_t* message, 
                 const int messageLen,
                 uint8_t* messageBuffer,
                 uint8_t* sigBuffer)
{
    uint8_t secretPadded[BLOCK_BYTES]; // Final secret must be a block size
    uint8_t opad[BLOCK_BYTES];
    uint8_t ipad[BLOCK_BYTES];

    for (int i = 0; i < BLOCK_BYTES; i++) {
        // No need to pad final key because everything is already init to 0
        // Just need to put original key or hashed key into the upper bits
        secretPadded[i] = 0;

        // Values defined in HMAC spec
        opad[i] = 0x5c;
        ipad[i] = 0x36;
    }

    if (secretLen > BLOCK_BYTES) {
        // Long secret get hashed down to 256-bits
        SHA256(secret, secretLen, secretPadded);
    } else {
        // Short secret get copied to buffer directly
        for (int i = 0; i < secretLen; i++) {
            secretPadded[i] = secret[i];
        }
    }

    #ifdef ENABLE_PRINT
    printf("Padded Secret:\n");
    for (int i = 0; i < BLOCK_BYTES; i += 4) {
        uint32_t key = expandCharArrayToInt(secretPadded, i);
        printf("%#02d %#010x\n", i, key);
    }
    #endif

    for (int i = 0; i < BLOCK_BYTES; i++) {
        opad[i] ^= secretPadded[i];
        ipad[i] ^= secretPadded[i];
    }

    for (int i = 0; i < BLOCK_BYTES; i++) {
        messageBuffer[i] = ipad[i];
    }
    for (int i = 0; i < messageLen; i++) {
        messageBuffer[BLOCK_BYTES + i] = message[i];
    }

    uint8_t hmacMid[BLOCK_BYTES + HASH_BYTES];
    #ifdef ENABLE_PRINT
    printf("SHA256(ipad || message)\n");
    #endif
    SHA256(messageBuffer, BLOCK_BYTES + messageLen, hmacMid);

    for (int i = 0; i < BLOCK_BYTES; i++) {
        messageBuffer[i] = opad[i];
    }
    for (int i = 0; i < HASH_BYTES; i++) {
        messageBuffer[BLOCK_BYTES + i] = hmacMid[i];
    }

    #ifdef ENABLE_PRINT
    printf("SHA256(opad || HASH)\n");
    #endif
    SHA256(messageBuffer, BLOCK_BYTES + HASH_BYTES, sigBuffer);
}

//-----------------------------------------------------------------------------
// Kernel
//-----------------------------------------------------------------------------

__kernel void bruteForceJWT(__global char *alpha, __global int *alphaLen, 
                            __global int *pswLen, __global char *message, 
                            __global int *msgLen, __global char *oriSig, 
                            __global int *oriSigLen, __global int *terminate)
{
    unsigned long num_items=1;
    for (int i=0; i<*pswLen; i++)
    {
        num_items = num_items*(*alphaLen+1);
    }
    unsigned long num_groups = get_num_groups (0);
    unsigned long id = get_global_id(0);
    int numElement = num_items/num_groups;

    if(num_items % num_groups != 0)
    {
        numElement += 1;
    }
    unsigned long startIdx = id*numElement;
    unsigned long endIdx = (id+1)*numElement-1;
    if(endIdx >= num_items)
    {
        endIdx = num_items - 1;
        if(startIdx > endIdx)
        {
            return;
        }
    }

//----copy message to local
    char local_message[500];
    int local_msgLen = *msgLen;

    for (int i = 0; i < local_msgLen; i++)
    {
        local_message[i] = message[i];
    }

//-----finishing copy message


//-----decoding
    int digits[10];
    char local_alpha[50];
    char ans[10];
    int ans_length = 0;
    for(int i = 0; i<*alphaLen; i++)
    {
        local_alpha[i] = alpha[i];
    }
    local_alpha[*alphaLen] = -1;

//--start trying
    unsigned long currIdx = startIdx;
    for(int z = 0; z < numElement; z++)
    {
        if(*terminate==1)
        {
            return;
        }
        int pos = 0;
        unsigned long temp = currIdx;
        while(temp!=0)
        {
            digits[pos] = temp % (*alphaLen+1);
            pos+=1;
            temp = temp/(*alphaLen+1);
        }

        ans_length = 0;
        for(int i = 0; i < pos; i++)
        {
            if((int)local_alpha[digits[i]] != -1)
            {
                ans[ans_length] = local_alpha[digits[i]];
                ans_length++;
            }
        }

        char messageBuffer[300];
        char signalBuffer[50];
        HMAC_SHA256((uint8_t*)ans, ans_length, (uint8_t*)local_message, local_msgLen, messageBuffer, signalBuffer);

        //check if signal match
        int found = 1;
        for(int i=0; i<(*oriSigLen); i++)
        {
            if(signalBuffer[i]!=oriSig[i])
            {
                found = 0;
            }
        }
        if(found == 1)
        {
            printf("found!\n");
            printf("pos:#%d\n", pos);
            printf("oriSigLen:#%d\n", *oriSigLen);
            for(int i = 0; i < ans_length; i++)
            {
                printf("decoded char:#%lu %c\n", currIdx, ans[i]);
            }
            *terminate = 1;
        }
        currIdx++;
    }
//---end trying

//----finish decoding

//printf("ID: %lu \n", id);
    if(0)
    {
        printf("num_items: %lu\n", num_items);
        printf("groups: %lu\n", num_groups);
        printf("global_id: %lu\n", id);
        printf("numElement: %d\n", numElement);
        printf("startIdx: %lu\n", startIdx);
        printf("endIdx: %lu\n", endIdx);
        printf("decoded char: 1 %c 2 %c 3 %c 4 %c 5 %c 6 %c\n",
        local_alpha[digits[0]], local_alpha[digits[1]], local_alpha[digits[2]],
        local_alpha[digits[3]], local_alpha[digits[4]], local_alpha[digits[5]]);
    }


}
