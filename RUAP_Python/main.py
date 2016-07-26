import cv2
import numpy as np
import AzureWebService as aws

#Određivanje područja dlana
def getConvexHull(foreground,frame):
    #Delay za pozivanje procjene geste
    delay=300
    #Font za ispis na zaslon
    font=cv2.FONT_HERSHEY_SIMPLEX
    
    _,contours,_=cv2.findContours(foreground,cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)
    for i in range(0,len(contours)):
    #Zanemaruje mala područja
        for i in range(len(contours)):
            cnt=contours[i]
            area=cv2.contourArea(cnt)
            if(area>8000):
                cnt=contours[i]
                #crtanje okvira ruke
                hulls=cv2.convexHull(cnt)
                #crtanje konture
                cv2.drawContours(frame, hulls, -1, (0,255,0), 2)
                rect = cv2.minAreaRect(cnt)
                box=cv2.boxPoints(rect)
                box=np.int0(box)
                cv2.drawContours(frame,[box],0,(0,0,255),2)
                global frames
                frames+=1
                if (frames == delay):
                    #VEKTOR ZNACAJKI
                    width = int(rect[1][0])
                    height = int(rect[1][1])
                    area = int(rect[1][0])*int(rect[1][1])
                    #///////////////////////
                    x=aws.getPrediction(width,height,area)
                    if(x=='0'):
                        cv2.putText(frame,'Loser',(110,110),font,4,(255,0,0),2,cv2.LINE_AA)
                    elif(x=='1'):
                        cv2.putText(frame,'OK',(110,110),font,4,(255,0,0),2,cv2.LINE_AA)
                    elif(x=='2'):
                        cv2.putText(frame,'Peace',(110,110),font,4,(255,0,0),2,cv2.LINE_AA)
                    global frames
                    frames=0
    return frame;


def main():
    #Deklaracija globalne varijable za brojanje frameova
    global frames
    frames=0
    
    face_cascade=cv2.CascadeClassifier("C:\\haarcascade_frontalface_alt2.xml")
    cam=cv2.VideoCapture(0);
    if (cam.isOpened()==False):
        print ("Error initializing camera")
        return -1;
    
    #Prozor s prikazom kamere
    cv2.namedWindow("Original")
    while (True):
        ret,frame=cam.read()
	#Detekcija lica
        faces=face_cascade.detectMultiScale(frame, 1.1, 2)
	#Promjena formata boje
        foreground=cv2.cvtColor(frame,cv2.COLOR_BGR2YCrCb)
	#Podjela u 3 zasebna kanala
        channels=cv2.split(foreground)
	#Thresholding odvojenih kanala
        channels[0]=cv2.inRange(channels[0], 5,163)
        _,channels[0]=cv2.threshold(channels[0],0,255,cv2.THRESH_BINARY_INV)
        channels[1]=cv2.inRange(channels[1], 130,165)
        _,channels[1]=cv2.threshold(channels[1],0,255,cv2.THRESH_BINARY)
        channels[2]=cv2.inRange(channels[2], 128,129)
        _,channels[2]=cv2.threshold(channels[2],0,255,cv2.THRESH_BINARY)
	#Otklanjanje suma - Morfoloske operacije
        kernel=np.ones((3,3),np.uint8)
        for i in range (0,3):
            channels[i]=cv2.erode(channels[i],kernel,iterations=1)
            channels[i]=cv2.dilate(channels[i],kernel,iterations=2)
	#spajanje kanala u jedan
        temp=cv2.bitwise_and(channels[0], channels[1])
        foreground=cv2.bitwise_and(temp, channels[2])
        #Morfologija
        foreground=cv2.erode(foreground, kernel,iterations=2)
        foreground=cv2.dilate(foreground, kernel,iterations=2)
	#Zamucenje za popunjavanje konture
        foreground=cv2.GaussianBlur(foreground,(5,5),0)
        foreground=cv2.erode(foreground, kernel,iterations=1)
        foreground=cv2.dilate(foreground, kernel,iterations=1)
	#Uklanjanje lica
        for (x,y,w,h) in faces:
            cv2.rectangle(temp,(x,y),(x+w,y+h),(0, 0, 0), -1)
        frame = getConvexHull(temp, frame)
        cv2.imshow("YCrCb", foreground)
        cv2.imshow("Prvi kanal", channels[0])
        cv2.imshow("Drugi kanal", channels[1])
        cv2.imshow("Treci kanal", channels[2])
        cv2.imshow("Temp", temp)
        cv2.imshow("Original", frame)
        if (cv2.waitKey(30) == 27):
            print("esc key is pressed by user")
            cv2.destroyAllWindows()
            break
