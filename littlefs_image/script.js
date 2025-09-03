let counter = 0;

// Função para incrementar o contador
function incrementCounter() {
    counter++;
    
    // Resetar para 1 quando chegar a 10
    if (counter > 10) {
        counter = 1;
    }
    
    // Atualizar o elemento na página
    const counterElement = document.getElementById('counter');
    if (counterElement) {
        counterElement.textContent = counter;
    }
    
    // Log no console para debug
    console.log(`Contador: ${counter}`);
}

// Inicializar quando a página carregar
document.addEventListener('DOMContentLoaded', function() {
    // Mostrar valor inicial
    incrementCounter();
    
    // Incrementar a cada 1 segundo
    setInterval(incrementCounter, 1000);
    
    console.log('Contador iniciado! Incrementando de 1 a 10...');
});