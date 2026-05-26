
let visitas = 1;


const contenedor = document.createElement("div");
contenedor.id = "info-js";
contenedor.innerHTML = `
    <h2>Informacion del servidor</h2>
    <p>Este contenido fue generado por JavaScript</p>
    <p>Visitas en esta sesion: <strong id="contador">${visitas}</strong></p>
    <button onclick="incrementar()">Incrementar visitas</button>
    <p>Hora de carga: <strong>${new Date().toLocaleTimeString()}</strong></p>
    <p>Navegador: <strong>${navigator.userAgent.split(" ")[0]}</strong></p>
`;
document.body.appendChild(contenedor);

function incrementar() {
    visitas++;
    document.getElementById("contador").textContent = visitas;
}