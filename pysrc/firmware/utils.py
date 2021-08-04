import time as utime
import gc
import uasyncio as asyncio
import sys
import ulogging as logging

LOG = logging.getLogger("utils")


__stats = {}
cnt = 0


def stats_function(f, *args, **kwargs):

    def new_func(*args, **kwargs):
        clear_stats()
        result = f(*args, **kwargs)
        dump_stats()
        return result
    return new_func


def profiled_function(f, *args, **kwargs):

    def new_func(*args, **kwargs):
        gc.collect()
        mem1 = gc.mem_free()

        result = f(*args, **kwargs)

        gc.collect()
        mem2 = gc.mem_free()
        LOG.debug("--> Memory took {:n} Bytes --> total Memory Free({:n}B {:.2f}kB)".format(
            mem1-mem2, mem2, mem2/1024))
        return result
    return new_func


def timed_function(f, *args, **kwargs):
    myname = str(f).split(' ')[1]

    def new_func(*args, **kwargs):
        t = utime.ticks_us()
        global cnt, __stats
        cnt += 1
        result = f(*args, **kwargs)
        delta = utime.ticks_diff(utime.ticks_us(), t)
        LOG.debug('{}: Function {} Time = {:6.3f}ms'.format(cnt, myname, delta/1000))
        if myname in __stats.keys():
            (c, t) = __stats[myname]
            __stats[myname] = (c+1, t+(delta/1000))
        else:
            __stats[myname] = (1, delta/1000)
        return result
    return new_func


def traced_function(f, *args, **kwargs):
    myname = str(f).split(' ')[1]

    def new_func(*args, **kwargs):
        t = utime.ticks_us()
        global cnt, __stats
        cnt += 1
        result = f(*args, **kwargs)
        delta = utime.ticks_diff(utime.ticks_us(), t)
        if myname in __stats.keys():
            (c, t) = __stats[myname]
            __stats[myname] = (c+1, t+(delta/1000))
        else:
            __stats[myname] = (1, delta/1000)
        return result
    return new_func


def clear_stats():
    global __stats, cnt
    __stats = {}
    cnt = 0


def dump_stats():
    global __stats, cnt

    LOG.info("Stats !!")
    LOG.info("------------")
    stats = {}
    for (k, v) in __stats.items():
        (c, t) = v
        LOG.info("{:20s} : {:8.2f}ms ({} calls) {:.2f} ms/call".format(k, t, c, t/c))


def clean_memory(console: logging.Logger = None):
    mem1 = gc.mem_free()
    gc.collect()
    mem2 = gc.mem_free()
    if console:
        console.debug("--> Memory freed {:n} Bytes --> total Memory Free({:n}B {:.2f}kB)".format(mem2-mem1, mem2, mem2/1024))


async def create_repeat_routine(f, delay_ms, *args, **kwargs):
    while True:
        f(*args, **kwargs)
        await asyncio.sleep_ms(delay_ms)


async def create_coroutine(f, delay_ms, *args, **kwargs):
    f(*args, **kwargs)
