from PIL import Image
import argparse
from os import listdir
from os.path import isfile, join

import serial
import time

def ConvertImage(imageName, inversed = False):
    imageIn = Image.open(imageName)
    imageIn = imageIn.resize((96, 96))
    imageGray = imageIn.convert('L')
    imageBw = imageGray.point(lambda x: 0 if x<128 else 1, '1')
    print(imageBw)
    r_data = imageBw.getdata()
    l_data = list(r_data)
    #Horisontal to vertical:
    verticalArr = []
    for col in range(0,96):
        for row in range(0, 96):
            currentCell = l_data[row * 96 +  col]
            if inversed == False:
                verticalArr.append(currentCell)
            else:
                if currentCell == 0:
                    verticalArr.append(1)
                else:
                    verticalArr.append(0)
    hex_arr = []
    for i in range(0,len(verticalArr),8):
        hex_num = 0
        for k in range(0,8):
            hex_num = hex_num + verticalArr[i+k]*(2**(7-k))
        hex_arr.append(hex_num)
    print("Hex array size: %d" % (len(hex_arr)))
    return hex_arr

def saveHexArray(array, outfile):
    f = open(outfile, "w")
    array_size = len(array)
    for point in range(0, array_size):
        if point != 0 and point % 12 == 0:
            f.write('\n')
        strHex = "0x%0.2X" % array[point]
        f.write(strHex)
        if point < array_size - 1:
            f.write(", ")
    f.close()

def saveByteArray(array, outfile):
    byteArray = bytearray(array)
    f = open(outfile, 'wb')
    f.write(byteArray)
    f.close()

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input')
    parser.add_argument('-o', '--output')
    parser.add_argument('--dirin')
    parser.add_argument('--dirout')   
    parser.add_argument('--inv', action='store_true')
    parser.add_argument('--byte', action='store_true')
    parser.add_argument('--port') 
    parser.add_argument('--fps', default=25) 

    args = parser.parse_args()

    inputFile = args.input
    outputFile = args.output

    if inputFile != None and outputFile != None:
        print(inputFile)
        hex_array = ConvertImage(inputFile, args.inv)
        if args.byte == True:
            saveByteArray(hex_array, outputFile)
        else:
            saveHexArray(hex_array, outputFile)
    if args.dirin != None and args.dirout != None:
        files = [f for f in listdir(args.dirin) if isfile(join(args.dirin, f))]
        print(files)
        for input_file in files:
            inputFile = join(args.dirin, input_file)
            outputFile = join(args.dirout, input_file[:-4])
            print(inputFile)
            hex_array = ConvertImage(inputFile, True)
            if args.byte == True:
                saveByteArray(hex_array, outputFile)
            else:
                saveHexArray(hex_array, outputFile)
    if args.dirout != None and args.port != None:
        time_period = 1000/args.fps #in ms

        current_milli_time = lambda: int(round(time.time() * 1000))

        files = [f for f in listdir(args.dirout) if isfile(join(args.dirout, f))]
        try:
            ser = serial.Serial(args.port, 512000, timeout=0)
            time.sleep(2);
            previous_time = current_milli_time()
            for outputFile in files:
                current_time = current_milli_time()
                while current_time - previous_time < time_period:
                    pass
                ser.write(open(join(args.dirout, outputFile),"rb").read())
        except serial.serialutil.SerialException:
            print('no COM port connection')


#
#
#filter = hid.HidDeviceFilter(vendor_id = 0x2047, product_id = 0x3ED)
#hid_device = filter.get_devices()
#device = hid_device[0]
#device.open()
#print(hid_device)

#target_usage = hid.get_full_usage_id(0x00, 0x3f)
#device.set_raw_data_handler(sample_handler)
#print(target_usage)


#report = device.find_output_reports()

#print(report)
#print(report[0])

#buffer = [0xFF]*64
#buffer[0] = 63

#print(buffer)

#report[0].set_raw_data(buffer)
#report[0].send()
