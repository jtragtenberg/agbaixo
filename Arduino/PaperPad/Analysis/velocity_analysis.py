import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

def load_csv(filename):
    """
    Load the CSV file into a pandas DataFrame.
    """
    try:
        data = pd.read_csv(filename)
        return data
    except Exception as e:
        print(f"Error loading CSV file: {e}")
        return None

def EWMA(input_value, filtered_value, alpha):
    """
    Exponential Weighted Moving Average (EWMA).
    
    Args:
    input_value (float): Current input value.
    filtered_value (float): Previously filtered value.
    alpha (float): Smoothing factor.
    
    Returns:
    float: The new filtered value.
    """
    return ((1 - alpha) * input_value) + (alpha * filtered_value)

def ADSR(input_values, alpha1, alpha2):
    """
    Filters the input signal using exponential averaging depending on whether the signal is rising or decaying.
    
    Args:
    input_values (list or np.array): The input signal values.
    alpha1 (float): Smoothing factor for rising signal.
    alpha2 (float): Smoothing factor for decaying signal.
    
    Returns:
    np.array: The filtered signal.
    """
    if len(input_values) < 2:
        raise ValueError("The input vector must have at least 2 elements.")
    
    ADSR_buffer = [0, 0]
    output_values = []
    output = 0
    
    for input_value in input_values:
        ADSR_buffer[1] = ADSR_buffer[0]
        ADSR_buffer[0] = input_value
        if ADSR_buffer[1] < ADSR_buffer[0]:
            output = EWMA(ADSR_buffer[0], output, alpha1)
        else:
            output = EWMA(ADSR_buffer[0], output, alpha2)
        output_values.append(output)
    
    return np.array(output_values)

def derivative(input, scale=3):
    """
    Calculate the derivative of sequence.
    """
    if len(input[::scale]) < 5:
        raise ValueError("The input vector must have at least 5 elements.")

    derivative_buffer = [0] * 5
    derivatives = []
    index = []
    aux = 0

    for newavg in input[::scale]:
        if aux == 0:
            index.append(0)
        else:
            index.append(aux * scale - 1)
        aux += 1

        for k in range(4, 0, -1):
            derivative_buffer[k] = derivative_buffer[k - 1]
        derivative_buffer[0] = newavg

        if len(derivatives) >= 4:
            derivative = (2.08 * derivative_buffer[0] - 
                          4 * derivative_buffer[1] + 
                          3 * derivative_buffer[2] - 
                          1.33 * derivative_buffer[3] + 
                          0.25 * derivative_buffer[4])
            derivatives.append(derivative)
        else:
            derivatives.append(0)

    return np.array(derivatives), np.array(index)

def vel_der(derivative, pressure, derthresh, pthresh, winsize=5, dscale=3, debouncesmps=100, type = 'pressure'):
    """
    Estimate the velocity of a derivative sequence by its peak.
    """
    if len(derivative) < winsize:
        raise ValueError("The derivative vector must have at least winsize number of elements.")
    
    detection_buffer = [0] * winsize
    vwinsize = 25
    value_buffer = [0] * vwinsize
    v = []
    index = []
    debounce = debouncesmps
    noteon = False

    for i in range(len(derivative)):
        for j in range(winsize-1, 0, -1):
            detection_buffer[j] = detection_buffer[j-1]
        detection_buffer[0] = derivative[i]

        for k in range(vwinsize-1, 0, -1):
            value_buffer[k] = value_buffer[k-1]
        try: 
            value_buffer[0] = pressure[i*3 - 1]
        except:
            value_buffer[0] = pressure[0]

        middle_index = len(detection_buffer) // 2
        if max(detection_buffer) == detection_buffer[middle_index] and np.all(np.array(detection_buffer) >= derthresh) and debounce >= debouncesmps and not noteon:
            if (type=='pressure'):
                v.append(value_buffer[0]-value_buffer[-1])
            else:
                v.append(detection_buffer[middle_index])
            index.append((i - middle_index) * dscale - 1)
            debounce = 0
            noteon = True
        elif pressure[(i * dscale) - (1 + middle_index)] <= pthresh:
            noteon = False
        debounce += 1
    
    return np.array(v), np.array(index)



'''
def vel_diff(input, thresh, winsize):
    """
    Estimate the velocity of a derivative sequence by its peak.
    """
    if len(input) < winsize:
        raise ValueError("The input vector must have at least 9 elements.")
    
    detection_buffer = [0] * winsize
    v = []
    index = []
    debouncesmps = 100

    for i in range(len(input)):
        newsmp = input[i]
        for j in range(winsize-1, 0, -1):
            detection_buffer[j] = detection_buffer[j-1]
        detection_buffer[0] = newsmp

        diff = detection_buffer[0] - detection_buffer[-1]

        if diff >= thresh and debouncesmps >= 100:
            v.append(diff)
            index.append(i)
            debouncesmps = 0
        debouncesmps += 1
    
    return np.array(v), np.array(index)
'''



def plot_data(xaxis, yaxis):
    """
    Plot the data from the DataFrame.
    """
    filtered_data = ADSR(yaxis, 0.6, 0.9)
    der, der_index = derivative(filtered_data)
    der_data = ADSR(der, 0.6, 0.95)
    vel1, index1 = vel_der(der_data, filtered_data, 1, 5)
    print(vel1)
    plt.close()
    plt.figure(figsize=(10, 5))
    plt.plot(xaxis, yaxis, label='Raw data', marker='.')
    plt.plot(xaxis, filtered_data, label='EWMA Pressure', marker='.')
    plt.plot(xaxis[der_index], der_data, label='Derivative', marker='.')
    for i, k in enumerate(xaxis[index1]):
        plt.plot(k, vel1[i], label='Velocity1', marker='*', markersize=15)
    
    plt.title('MIDI Control Change Messages')
    plt.xlabel('Time (s)')
    plt.ylabel('Value')
    plt.legend()
    plt.grid(True)

if __name__ == "__main__":
    csv_filename = 'miscellaneous.csv'
    data = load_csv(csv_filename)
    if data is not None:
        plot_data(data['time'], data['data_value'])
        plt.show()
