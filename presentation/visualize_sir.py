import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import glob
import os
import struct

try:
    import imageio
except ImportError:
    imageio = None

# Configuration
OUTPUT_FILENAME = 'sir_simulation.mp4'
INPUT_DIR = 'output_states'

def read_bin_file(filename):
    with open(filename, 'rb') as f:
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
search_path = os.path.join(INPUT_DIR, "output_step_*.bin")
files = sorted(glob.glob(search_path), key=lambda x: int(os.path.basename(x).split('_')[2].split('.')[0]))

if not files:
    print(f"No output files found in {search_path}. Make sure the folder exists and contains .bin files.")
    exit()

print(f"Found {len(files)} frames.")

# Create a custom frame sequence to control pacing
frame_indices = []

# 1. Freeze Step 0 for 1 second (approx 15 frames at 15 FPS)
if len(files) > 0:
    frame_indices.extend([0] * 15)

# 2. Slow down the first 20 frames (show each 5 times)
slow_count = min(20, len(files))
for i in range(slow_count):
    frame_indices.extend([i] * 5)

# 3. Play the rest at normal speed
frame_indices.extend(range(slow_count, len(files)))

print(f"Generated animation sequence with {len(frame_indices)} frames.")

# Setup Plot
fig, ax = plt.subplots(figsize=(8, 8))
ax.axis('off')

# Initial frame
state, density = read_bin_file(files[0])
img_data = get_color_grid(state, density)
im = ax.imshow(img_data, interpolation='nearest')
title = ax.set_title("Step 0")

def update_plot(frame_idx):
    filename = files[frame_idx]
    step_num = os.path.basename(filename).split('_')[2].split('.')[0]
    
    state, density = read_bin_file(filename)
    img_data = get_color_grid(state, density)
    
    im.set_data(img_data)
    title.set_text(f"Step {step_num}")
    return im, title

# Animation or Video Generation
if OUTPUT_FILENAME.endswith('.mp4'):
    if imageio is None:
        print("Error: 'imageio' library is required for MP4 output.")
        print("Please run: pip install imageio[ffmpeg]")
        exit()

    print(f"Saving video to {OUTPUT_FILENAME}...")
    with imageio.get_writer(OUTPUT_FILENAME, fps=15) as writer:
        for i, idx in enumerate(frame_indices):
            if i % 10 == 0:
                print(f"Rendering frame {i}/{len(frame_indices)}")
            
            update_plot(idx)
            
            # Draw the figure to a numpy buffer
            fig.canvas.draw()
            image = np.frombuffer(fig.canvas.tostring_rgb(), dtype='uint8')
            image = image.reshape(fig.canvas.get_width_height()[::-1] + (3,))
            
            writer.append_data(image)
            
    print(f"Saved to {OUTPUT_FILENAME}")

else:
    # Fallback to GIF using matplotlib animation
    def update(frame_idx):
        if frame_idx % 10 == 0:
            print(f"Processing file index {frame_idx}")
        return update_plot(frame_idx)

    ani = animation.FuncAnimation(fig, update, frames=frame_indices, interval=50, blit=True)

    print("Saving animation...")
    ani.save(OUTPUT_FILENAME, writer='pillow', fps=15)
    print(f"Saved to {OUTPUT_FILENAME}")

plt.close()
