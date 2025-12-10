import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import glob
import os
import struct

# Configuration
OUTPUT_FILENAME = 'sir_simulation.gif'

def read_bin_file(filename):
    with open(filename, 'rb') as f:
        # Read dimensions
        w_bytes = f.read(4)
        h_bytes = f.read(4)
        w = struct.unpack('i', w_bytes)[0]
        h = struct.unpack('i', h_bytes)[0]
        
        size = w * h
        
        # Read state
        state_bytes = f.read(size)
        state = np.frombuffer(state_bytes, dtype=np.uint8).reshape((h, w))
        
        # Read density
        density_bytes = f.read(size * 4)
        density = np.frombuffer(density_bytes, dtype=np.float32).reshape((h, w))
        
    return state, density

def get_color_grid(state, density):
    
    h, w = state.shape
    rgb = np.zeros((h, w, 3), dtype=np.uint8)
    
    # Masks
    mask_sus = (state == 0)
    mask_inf = (state == 1)
    mask_res = (state == 2)
    mask_water = (density == 0.0)
    
    # Apply colors
    # Susceptible: Yellow
    rgb[mask_sus] = [255, 255, 100] 
    
    # Infectious: Red
    rgb[mask_inf] = [200, 50, 50]
    
    # Resistant (Recovered): Green
    rgb[mask_res] = [50, 150, 50]
    
    # Water: Blue (Overwrites resistant if density is 0)
    rgb[mask_water] = [50, 100, 200]
    
    return rgb

# Get list of files
files = sorted(glob.glob("output_step_*.bin"), key=lambda x: int(x.split('_')[2].split('.')[0]))

if not files:
    print("No output files found. Run the C++ simulation first.")
    exit()

print(f"Found {len(files)} frames.")

# Setup Plot
fig, ax = plt.subplots(figsize=(8, 8))
ax.axis('off')

# Initial frame
state, density = read_bin_file(files[0])
img_data = get_color_grid(state, density)
im = ax.imshow(img_data, interpolation='nearest')
title = ax.set_title("Step 0")

def update(frame_idx):
    filename = files[frame_idx]
    step_num = filename.split('_')[2].split('.')[0]
    
    state, density = read_bin_file(filename)
    img_data = get_color_grid(state, density)
    
    im.set_data(img_data)
    title.set_text(f"Step {step_num}")
    
    if frame_idx % 10 == 0:
        print(f"Processing frame {frame_idx}/{len(files)}")
    return [im, title]

ani = animation.FuncAnimation(fig, update, frames=len(files), interval=50, blit=True)

print("Saving animation...")
ani.save(OUTPUT_FILENAME, writer='pillow', fps=15)
print(f"Saved to {OUTPUT_FILENAME}")
plt.close()
