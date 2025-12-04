import matplotlib.pyplot as plt
import matplotlib
matplotlib.use("Agg")  

# ---------------------- PID Controller ----------------------
class PID:
    def __init__(self, Kp, Ki, Kd, dt=0.5, integrator_limit=100.0):
        self.Kp = Kp
        self.Ki = Ki
        self.Kd = Kd
        self.dt = dt
        
        self.integral = 0.0
        self.previous_error = 0.0
        self.integrator_limit = integrator_limit

    def compute(self, setpoint, measurement):
        error = setpoint - measurement

        # Termo proporcional
        P = self.Kp * error

        # Integral com anti-windup
        self.integral += error * self.dt
        self.integral = max(-self.integrator_limit, min(self.integrator_limit, self.integral))
        I = self.Ki * self.integral

        # Derivativo
        derivative = (error - self.previous_error) / self.dt
        D = self.Kd * derivative

        self.previous_error = error

        return P + I + D


# ---------------------- SIMULAÇÃO ----------------------
pid = PID(Kp=0.5, Ki=0.1, Kd=0.5, dt=0.5)

setpoint = 37.0      # temperatura alvo
temp = 25.0          # temperatura inicial

temps = []
times = []

# parâmetros térmicos
ambient = 25.0
R = 5.0              # resistência térmica
C = 5.0              # capacidade térmica
dt = 0.5

for i in range(500):
    # PID calcula potência de aquecimento
    control = pid.compute(setpoint, temp)

    # perdas para ambiente
    power_loss = (temp - ambient) / R

    # potência líquida no sistema
    net_power = control - power_loss

    # variação de temperatura pelo modelo RC
    dT = (net_power / C) * dt
    temp += dT

    # salvar histórico
    temps.append(temp)
    times.append(i * dt)


# ---------------------- GRÁFICO ----------------------
plt.figure(figsize=(10,4))
plt.plot(times, temps, label="Temperatura", linewidth=2)
plt.plot(times, [setpoint]*len(times), "--", label="Setpoint (37°C)")

plt.xlabel("Tempo (s)")
plt.ylabel("Temperatura (°C)")
plt.title("Simulação do Controle PID da Incubadora")
plt.grid(True)
plt.legend()

plt.savefig("grafico_pid.png", dpi=300)   
print("Gráfico salvo como grafico_pid.png")
