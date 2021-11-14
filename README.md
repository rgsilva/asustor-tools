


# Asustor Tools

This is a repository for storing some of my personal tools for hacking, modding and/or
reverse engineering Asustor's NAS operating system. Anything you see here is free to use,
just give me credits :)

# Disclaimer

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

**TL;DR: if something gets on fire, it's your fault. Sorry.**

# The tools

## fan-control

Background story: [Reverse engineering and fine-tuning Asustor NAS fans](https://blog.rgsilva.com/reverse-engineering-and-fine-tuning-asustor-nas-fans/)

This is a simple tool to provide you a better control of your NAS fans. For some reason
these devices can get hotter than normal and somehow the OS is not properly controlling
the fans, so this tool can help with that. Or you just want your NAS to be really quiet,
or really cold. Anyway, this is the tool for you.

```
./asustor-fan-control <fans> <sleep> <curve>

fans:  comma-separated fan indexes
  ex:  0,1,2,3
sleep: nanoseconds to sleep between each PWM check
  ex:  10000
curve: comma-separated temp:pwm ordered by temp
  ex:  30:0,40:50,50:100,60:150,70:255
```

It's not a daemon yet, so you need to run this inside a screen or something. It also requires
firmware version 4.0.0.RN3 or similar/higher because you need to link to `libnhal.so` with
the function `Hal_Disk_Get_Temperature` exported. It was tested only on an AS3104T.

### Missing features

1. This tool does not support NVMe drives.
2. This tool is getting the available drives by brute-forcing it. We should be able to find them
   using the NAS functions.
3. No easy install yet (either a full application or a service).
4. No configuration file (everything is in the arguments for now).

# How to build the tools?

This is tricky, but I have a Docker container for that which I'll publish a compose
eventually. Right now you can build the tools by doing the following:

1. Create an Ubuntu 20.04 LTS Docker container, mounting the following folders:
    *  `/usr/lib` as `/mnt/lib`
    * `/root/ubuntu` as `/root/nas`
2. Use the `build.sh` script I provided in this repository. It'll try and link the provided file
  to anything everything. This should work just fine.
3. The tool should be available at `/root/ubuntu`.

# FAQ

1. Is this legal?
  It depends on your location. Since I'm not providing any source code to anything I've been
working on besides the tools, I believe it's still legal.

2. How do I contact you?
  You don't. But if you must, GitHub is your friend and I'm also trackable in Twitter.

3. Why this repository?
  I have way too many tools already from my recent experimentation on reverse engineering
Asustor's NAS firmware. I actually own one of their products and I quite enjoy it, so
adding features, even if it requires some hard work, is worth sometimes!

4. Tell me more!
  Check [my blog](https://blog.rgsilva.com/), I have a bunch of posts playing with my NAS! :)
