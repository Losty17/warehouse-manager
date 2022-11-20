# warehouse-manager
Projeto final da cadeira de Programação para Resolução de Problemas

## Opções

1. Manutenção Corretiva.
    - Mecânico: Solicita material pelo nome
    - Almoxarifado: 
        1. Entrar com ID do requisitante.
        2. Procura o item e passa a quantidade desejada. 
        3. Confirma a entrega e gera uma Ordem de Serviço/Manutenção

    - Mecânico:
        1. Recebe uma pendência para dar continuidade na Ordem de Manutenção
        2. Detalhar a solução do problema, os materiais utilizados, as quantidades, etc;
        3. Em caso de divergência, enviar notificação ao almoxarifado para realizar contagem do estoque;
        4. Enviar notificação ao chefe da manutenção com o número da OM com observações se houver divergências.
        5. O mecânico tem 48h para responder a OM, caso não responda, uma notificação é enviada ao chefe.

2. Manutenção Preventiva

3. Estoque
    - Faz listagem do estoque, usando uma matriz para informar posição dos itens na estante, além de id e quantidade

4. Histórico de OM
    - Mostra as últimas ordens de manutenção

- Utilizar coordenadas de estante como uso de matrizes

- TALVEZ FAZER UMA GUI E USAR BANCO DE DADOS

- 3 tipos de rolamento, 3 tipos de correia, 3 tipos de parafuso, 3 tipos de porca, 3 tipos rebite
