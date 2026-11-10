#!/usr/bin/env python3
"""Chart composite FPS (one frame = all envs tiled) on H100: CPU vs GPU readback vs GPU no-readback."""
import re
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

envs = [1,4,8,16,32,64,128,256]
def parse(f): return [float(x) for x in re.findall(r'(\d+\.\d)\b', open(f).read())]
cpu = parse('benchmarks/h100_render_cpu_2026-11-06.txt')
rb  = parse('benchmarks/h100_render_gpu_readback_on_2026-11-10.txt')
norb= parse('benchmarks/h100_render_gpu_readback_off_2026-11-10.txt')

def best(vals, i): return max(vals[i*5:(i+1)*5])
cpu_f = [best(cpu, i) for i in range(8)]
rb_f  = [best(rb, i) for i in range(8)]
norb_f= [best(norb, i) for i in range(8)]

fig, ax = plt.subplots(figsize=(11, 6))
x = range(len(envs))
w = 0.27
b1 = ax.bar([i-w for i in x], cpu_f, width=w, label='CPU software', color='#4C72B0')
b2 = ax.bar([i for i in x],   rb_f,  width=w, label='GPU EGL + readback (pixels to CPU)', color='#DD8452')
b3 = ax.bar([i+w for i in x], norb_f,width=w, label='GPU EGL no readback (GPU only)', color='#55A868')
ax.set_yscale('log')
ax.set_xticks(list(x)); ax.set_xticklabels([str(e) for e in envs])
ax.set_xlabel('num envs (one composite frame = all envs tiled)')
ax.set_ylabel('composite frames/s (log scale)')
ax.set_title('Griddly tiled render, H100 — composite FPS (spiders lvl4)\nreadback = one GPU->CPU sync of the whole frame each frame')
for bars in (b1, b2, b3):
    for r in bars:
        h = r.get_height()
        ax.text(r.get_x()+r.get_width()/2, h*1.2, f'{h:,.0f}' if h>=1000 else f'{h:.1f}',
                ha='center', fontsize=7, rotation=90)
ax.legend(); ax.grid(True, which='both', alpha=0.3)
fig.tight_layout()
out = 'benchmarks/h100_render_composite_fps_2026-11-10.png'
fig.savefig(out, dpi=130)
print('saved', out)
