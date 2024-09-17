
# HvcNope Demo - Minifilter Evasion

This demo shows the utility of arbitrary kernel function calling.

The idea is simple - evade defender's minifilter, `WdFilter.sys`.

## Defender Evasion
`WdFilter.sys` is defender's component responsible for realtime filesystem-related detections. It utilizes a minifilter driver.

### How Does a Minifilter Work?
Briefly, before an IRP goes to `Ntfs.sys` (or any other filesystem driver, such as `Npfs.sys` for named pipes, etc), it goes through an upper device object belonging to `FltMgr.sys`.

`FltMgr.sys` calls different operations that were registered in different minifilters (such as `WdFilter.sys`) to intercept IRPs. An example would be `WdFilter.sys` intercepting `IRP_MJ_WRITE` requests to scan for malicious content.

### What Can We Do About It?
Let's just call `Ntfs.sys`'s IRP handler directly! We'll craft the IRP ourselves, hence skipping `FltMgr.sys` and defender entirely.

## The Demo
We use EICAR to simulate malicious content.

We first write it into `Test1.txt` using normal usermode APIs, and `WdFilter.sys` should catch it.

But then, we run the kernel mode test - we craft an `IRP_MJ_WRITE` request and send it.

It should be noted that `Ntfs!NtfsFsdWrite`'s address should be specified, as manually resolving it was a bit of a pain ;)