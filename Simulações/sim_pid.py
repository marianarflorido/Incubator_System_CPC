import matplotlib.pyplot as plt

# PID class
class PIDController:
    def __init__(self, Kp, Ki, Kd, dt=0.5):
        self.Kp = Kp
        self.Ki = Ki
        self.Kd = Kd
        self.dt = dt
        self.integral = 0.0
        self.previous_error = 0.0

    def compute(self, setpoint, current_temperature):
        error = setpoint - current_temperature
        P = self.Kp * error
        self.integral += error * self.dt
        self.integral = max(min(self.integral, 100), -100)
        I = self.Ki * self.integral
        derivative = (error - self.previous_error) / self.dt
        D = self.Kd * derivative
        output = P + I + D
        self.previous_error = error
        return output

pid = PIDController(0.5, 0.1, 0.5, dt=0.5)

setpoint = 37.0
temp = 25.0

temps = []
times = []

for i in range(500):
    pid.compute(setpoint, temp)

    temps.append(temp)
    times.append(i * 0.5)

    ambient = 25.0
    R = 5.0
    C = 5.0
    power_loss = (temp - ambient) / R
    net_power = pid.compute(setpoint, temp) - power_loss
    dT = (net_power / C) * 0.5
    temp += dT

plt.figure(figsize=(8,4))
plt.plot(times, temps)
plt.plot(times, [setpoint]*len(times), linestyle="--")
plt.xlabel("Tempo (s)")
plt.ylabel("Temperatura (°C)")
plt.title("Resposta do Sistema com PID")
plt.show()
