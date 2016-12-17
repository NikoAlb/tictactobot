import numpy as np
import cv2
class Image():
    def __init__(self):
        cv2.ocl.setUseOpenCL(False)
        self.calibrated = False
        self.orig = []
        self.transform = []
        self.gray = []
        self.adap_thresh = []
        self.left_region_bounds = np.zeros([2,2])
        self.left_region = []
        self.width = 0
        self.height = 0
        self.sidelength = 0
        self.moving_min_area = 1000.0
        self.token_min_area = 1200.0
        self.tile_min_area = 300.0
        self.fgbg = cv2.createBackgroundSubtractorMOG2()
        
    def __enter__(self):
        self.cap = cv2.VideoCapture(0)
        return self
        
    def __exit__(self, exception, value, traceback):
        self.cap.release()
        
    def __record_frame(self):
        recording, self.orig = self.cap.read()
        if not recording:
            raise Exception('ERROR: Cannot access camera.')
        if self.calibrated:
            self.transform = cv2.warpPerspective(self.orig,self.transformationMatrix,(self.sidelength,self.sidelength))
            self.left_region = self.orig[self.left_region_bounds[0,1]:self.left_region_bounds[1,1],self.left_region_bounds[0,0]:self.left_region_bounds[1,0]]
        else:
            # Set dimensions of transformed image
            self.height, self.width, ch = self.orig.shape
            if (self.width > self.height):
                self.sidelength = self.height
            else:
                self.sidelength = self.width
            
    def show_frame(self):
        cv2.imshow('Frame captured',self.orig)
        cv2.waitKey(0)
        cv2.destroyAllWindows
    
    def show_transform(self):
        cv2.imshow('Transformed board',self.transform)
        cv2.waitKey(0)
        cv2.destroyAllWindows
            
    def calibrate(self):
        # Record two images, so that the camera adjusts shutter time
        self.__record_frame()
        self.__record_frame()
        #self.show_frame() #Debug
        
        # Convert image to binary
        gray = cv2.cvtColor(self.orig,cv2.COLOR_BGR2GRAY)
        bw = cv2.adaptiveThreshold(gray, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, cv2.THRESH_BINARY_INV, 123, 50)
        
        # Find the contours in the binary image        
        cnt_img, contours, hierarchy = cv2.findContours(bw,cv2.RETR_TREE,cv2.CHAIN_APPROX_SIMPLE)
        
        # Find inner contour which should be the middle field
        highest_hierarchy_level = -1
        for j in range(0,len(contours)):
            cnt = contours[j]
            hull = cv2.convexHull(cnt)
            area = cv2.contourArea(hull)
            # If area of enclosing contour is too small, skip it
            if (area<self.tile_min_area):
                continue
            # Check if the hierarchy is smaller than the last one.
            hier = hierarchy[0][j]
            if hier[3]>highest_hierarchy_level:
                highest_hierarchy_level = hier[3]
                inner_cnt = j
        
        # Fit a rectangle in the detected middle field
        cnt = contours[inner_cnt]
        x,y,w,h = cv2.boundingRect(cnt)
        
        # Save the lower right corner of the region left from the field
        # Here, the sign indicating if the player wants to start or not will be detected
        left_region_x = int(x-1.5*w)
        left_region_y = int(y+2.5*h)
        if (left_region_x<0):
            raise Exception ('Not able to see region left of the field. Adjust camera position.')
        if (left_region_y>self.height):
            left_region_y = int(self.height)
        self.left_region_bounds[1,:] = [left_region_x,left_region_y]
    
        # Calculate corner locations in source image from rectangle corners
        urSrc = [x+w,y]
        lrSrc = [x+w,y+h]
        llSrc = [x,y+h]
        ulSrc = [x,y]
        cornersSrc = np.float32([urSrc, lrSrc, llSrc, ulSrc])
        
        # Define corner positions in destination image
        urDst = [0.65*self.sidelength,0.35*self.sidelength]
        lrDst = [0.65*self.sidelength,0.65*self.sidelength]
        llDst = [0.35*self.sidelength,0.65*self.sidelength]
        ulDst = [0.35*self.sidelength,0.35*self.sidelength]
        cornersDst = np.float32([urDst, lrDst, llDst, ulDst])
        
        # Transform the image    
        self.transformationMatrix = cv2.getPerspectiveTransform(cornersSrc,cornersDst)
        self.transform = cv2.warpPerspective(self.orig,self.transformationMatrix,(self.sidelength,self.sidelength))
        self.calibrated = True
        
    def is_moving(self,region):
        '''region = 0: look at the field, region = 1: look at left region'''
        # http://docs.opencv.org/3.0-beta/doc/py_tutorials/py_imgproc/py_morphological_ops/py_morphological_ops.html
        kernel = np.ones((5,5),np.uint8)
        self.__record_frame()
        
        if (region==0):
            fgmask = self.fgbg.apply(self.transform)
        elif (region==1):
            fgmask = self.fgbg.apply(self.left_region)
        else:
            raise Exception ('Invalid region selected for movement detection')
        
        ret, bw = cv2.threshold(fgmask, 100, 255, cv2.THRESH_BINARY)
        opening = cv2.morphologyEx(bw, cv2.MORPH_OPEN, kernel)
        
        opening, contours, hierarchy = cv2.findContours(opening,cv2.RETR_EXTERNAL,cv2.CHAIN_APPROX_SIMPLE)
        
        # loop over the contours
        for cnt in contours:
            # if the contour is too small, ignore it
            if cv2.contourArea(cnt) < (1-0.9*region)*self.moving_min_area:
                continue
            else:
                print("Something is moving.")
                return True
        print('No movement detected.')
        return False
        
    def detect_sign(self,emptyTiles):
        # emptyTiles should be a list with empty tiles
        emptyTiles_shift = [x-1 for x in emptyTiles]
        
        
        while self.is_moving(0):
            cv2.imshow('Transformed board',self.transform)
            cv2.waitKey(500)
        
        self.__record_frame()
        cv2.imshow('Transformed board',self.transform)
        
        gray = cv2.cvtColor(self.transform,cv2.COLOR_BGR2GRAY)
        bw = cv2.adaptiveThreshold(gray, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, cv2.THRESH_BINARY_INV, 123, 50)
        
        # Define the regions of the tiles in the image
        y11 = x11 = 0
        y12 = x12 = int(0.3*self.sidelength)
        y21 = x21 = int(0.4*self.sidelength)
        y22 = x22 = int(0.6*self.sidelength)
        y31 = x31 = int(0.7*self.sidelength)
        y32 = x32 = int(self.sidelength)
        
        # Create a Region of Interest for each tile
        tile_ll = bw[y31:y32,x11:x12]
        tile_lm = bw[y31:y32,x21:x22]
        tile_lr = bw[y31:y32,x31:x32]
        tile_ml = bw[y21:y22,x11:x12]
        tile_mm = bw[y21:y22,x21:x22]
        tile_mr = bw[y21:y22,x31:x32]
        tile_ul = bw[y11:y12,x11:x12]
        tile_um = bw[y11:y12,x21:x22]
        tile_ur = bw[y11:y12,x31:x32]
        tiles = [tile_ll,tile_lm,tile_lr,tile_ml,tile_mm,tile_mr,tile_ul,tile_um,tile_ur]
        
        for i in emptyTiles_shift:
            tile = tiles[i]
            print('Looking for token in tile {}'.format(i+1))
            # cv2.imshow('tile',tile) # DEBUG
            
            # RETR_EXTERNAL
            # If you use this flag, it returns only extreme outer flags. All child contours are left behind. 
            # http://docs.opencv.org/trunk/d9/d8b/tutorial_py_contours_hierarchy.html
            cnt_img, contours, hierarchy = cv2.findContours(tile,cv2.RETR_EXTERNAL,cv2.CHAIN_APPROX_SIMPLE)
            
            tokens = list(contours)
            deleteCounter = 0
            
            for j in range(0,len(contours)):
                cnt = contours[j]
                hull_indices = cv2.convexHull(cnt,returnPoints = False)
                hull = cv2.convexHull(cnt)
                area = cv2.contourArea(hull)
                
                if (area<self.token_min_area):
                    tokens.pop(j-deleteCounter)
                    deleteCounter += 1
                    continue
                
                defects = cv2.convexityDefects(cnt, hull_indices)
                distances = np.array(defects[:,:,3])
                mean = np.mean(distances[1:])
                
                # shape detection from
                # http://stackoverflow.com/questions/31974843/detecting-lines-and-shapes-in-opencv-using-python
                # and http://docs.opencv.org/trunk/dd/d49/tutorial_py_contour_features.html
                if 1000<mean:
                    # CROSS
                    print('Found cross in tile {}'.format(i+1))
                    return [True,'X', i+1]
                else:
                    # CIRCLE
                    print('Found circle in tile {}'.format(i+1))
                    return [True,'O', i+1]
                   
        cv2.waitKey(500)
        return [False,'.',-1]
        
    def detect_first_move(self):
        if not self.calibrated:
            raise Exception ('Calibrate the camera before detecting the first move.')
        
        while (self.is_moving(1)):
            cv2.imshow('Left Region',self.left_region)
            cv2.waitKey(500)
        
        self.__record_frame()
        cv2.imshow('Left Region',self.left_region)
        gray = cv2.cvtColor(self.left_region,cv2.COLOR_BGR2GRAY)
        bw = cv2.adaptiveThreshold(gray, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, cv2.THRESH_BINARY_INV, 123, 50)
        
        # RETR_EXTERNAL
        # If you use this flag, it returns only extreme outer flags. All child contours are left behind. 
        # http://docs.opencv.org/trunk/d9/d8b/tutorial_py_contours_hierarchy.html
        cnt_img, contours, hierarchy = cv2.findContours(bw,cv2.RETR_EXTERNAL,cv2.CHAIN_APPROX_SIMPLE)
        
        for j in range(0,len(contours)):
            cnt = contours[j]
            hull_indices = cv2.convexHull(cnt,returnPoints = False)
            hull = cv2.convexHull(cnt)
            area = cv2.contourArea(hull)
            # Check if area covered by contour is big enough to be a token
            if (area<self.tile_min_area):
                continue
                
            defects = cv2.convexityDefects(cnt, hull_indices)
            distances = np.array(defects[:,:,3])
            # print('The distances in tile {} are:'.format(i))
            # print(distances)
            # print('The area of the token in tile {} is {}'.format(i,area))
            mean = np.mean(distances[1:])
            if 800<mean:
                # CROSS
                print('Found cross.')
                return [True,'X']
            else:
                # CIRCLEh
                print('Found circle.')
                return [True,'O']
                
        cv2.waitKey(500)
        return [False,'.']