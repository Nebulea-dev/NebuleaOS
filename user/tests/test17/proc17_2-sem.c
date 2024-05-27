#include "sysapi.h"
#include "test17-sem.h"

static int buf_receive(struct test17_buf_st *st)
{
        int x;
        assert(wait(st->rsem) == 0);
        assert(wait(st->mutex) == 0);
        x = 0xff & (int)(st->buf[(st->rpos++) % sizeof(st->buf)]);
        assert(signal(st->mutex) == 0);
        assert(signal(st->wsem) == 0);
        return x;
}

int main(void *arg)
{
        struct test17_buf_st *st = NULL;

        (void)arg;

        st = (struct test17_buf_st *)shm_acquire("test17_shm");
        printf("st = %p\n", st);
        assert(st != NULL);

        while (1)
        {
                int x = buf_receive(st);
                atomic_incr(&st->received[x]);
        }
        printf("release\n");
        shm_release("test17_shm");
        return 0;
}
