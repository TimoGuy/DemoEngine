from array import array
from multiprocessing.dummy import Array
import sys
from xmlrpc.client import Boolean
from PIL import Image
import numpy as np
import tkinter as tk
import tkinter.filedialog as tkfd
from random import random
#from perlin_numpy import ( generate_fractal_noise_2d )        # Run `pip3 install git+https://github.com/pvigier/perlin-numpy` for this package


IMAGE_CUTOUT_THRESHOLD = 130
CLUSTER_SIZE_THRESHOLD = 10
GREEN_OUT_COLOR = [0, 255, 0, 0]


root = tk.Tk()
root.withdraw()


def read_image(path: str) -> Image:
    try:
        image = Image.open(path)
        return image
    except Exception as e:
        print(e)


if __name__ == '__main__':
    #
    # Load in image
    #
    fname = tkfd.askopenfilename(initialdir='.')
    if not fname:
        sys.exit(0)

    image = read_image(fname)
    image.show()

    #
    # Change image to green out lower threshold (and make it transparent)
    # NOTE: this also builds up the image mask as well
    #
    img_mask = []
    arr = np.array(image)
    for i in range(len(arr)):
        img_mask_row = []
        for j in range(len(arr[i])):
            mask_val = True
            if arr[i][j][0] < IMAGE_CUTOUT_THRESHOLD:
                arr[i][j] = GREEN_OUT_COLOR
                mask_val = False
            img_mask_row.append(mask_val)
        img_mask.append(img_mask_row)

    image_greened = Image.fromarray(arr)
    image_greened.show()

    save_fname = tkfd.asksaveasfilename(initialdir='.', confirmoverwrite=True, initialfile='output_greened.png')
    if not save_fname:
        sys.exit(0)

    image_greened.save(save_fname)

    #
    # Gather together the values from the image mask
    # NOTE: this is destructive to img_mask btw
    #
    def check_img_mask_location(i: int, j: int) -> bool:
        if i < 0 or i >= len(img_mask) or \
            j < 0 or j >= len(img_mask[i]):
            return False
        return img_mask[i][j]

    # @DEPRECATED: python doesn't like recursion I guess eh
    def visit_img_mask_location(visiting_arr: array, i: int, j: int, i_prev: int = -1, j_prev: int = -1) -> bool:
        if i < 0 or i >= len(img_mask) or \
            j < 0 or j >= len(img_mask[i]):
            return False

        mask_val = img_mask[i][j]
        if mask_val:
            img_mask[i][j] = False  # Destruct this node bc it's visited now
            visiting_arr.append((i, j))
            if i+1 != i_prev or j != j_prev:
                visit_img_mask_location(visiting_arr, i+1, j, i, j)
            if i-1 != i_prev or j != j_prev:
                visit_img_mask_location(visiting_arr, i-1, j, i, j)
            if i != i_prev or j+1 != j_prev:
                visit_img_mask_location(visiting_arr, i, j+1, i, j)
            if i != i_prev or j-1 != j_prev:
                visit_img_mask_location(visiting_arr, i, j-1, i, j)
        return mask_val

    img_mask_clusters = []
    for i in range(len(img_mask)):
        for j in range(len(img_mask[i])):
            new_cluster = []

            # Try looking for any new clusters, and hope that the cells get added in
            # I feel kinda clever for coming up with this iterative solution hehehehe
            check_index = 0
            check_positions = []
            check_positions.append((i, j))
            while check_index < len(check_positions):
                pos = check_positions[check_index]
                check_i = pos[0]
                check_j = pos[1]
                if check_img_mask_location(check_i, check_j):
                    new_cluster.append((check_i, check_j))
                    img_mask[check_i][check_j] = False      # Destruct this node bc it's visited now

                    check_positions.append((check_i+1, check_j))        # Add more nodes to visit bc this was a success
                    check_positions.append((check_i-1, check_j))
                    check_positions.append((check_i, check_j+1))
                    check_positions.append((check_i, check_j-1))

                check_index += 1
            
            if len(new_cluster) > 0:
                img_mask_clusters.append(new_cluster)
    
    #
    # With the new clusters, color them with random colors
    #

    for cluster in img_mask_clusters:
        random_color = GREEN_OUT_COLOR if len(cluster) < CLUSTER_SIZE_THRESHOLD else [int(random() * 255), int(random() * 255), int(random() * 255), 255]
        for coordinate in cluster:
            i = coordinate[0]
            j = coordinate[1]
            arr[i][j] = random_color
    
    image_greened_clustered = Image.fromarray(arr)
    image_greened_clustered.show()

    save_fname2 = tkfd.asksaveasfilename(initialdir='.', confirmoverwrite=True, initialfile='output_clustered.png')
    if not save_fname2:
        sys.exit(0)

    image_greened_clustered.save(save_fname2)

