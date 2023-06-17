# LSEG-Flower-Exchange-System
Flower Exchange System was developed as a part of the LSEG C++ workshop series for UCSC first-year undergraduates.

- This C++ code intends to achieve minimum transaction time for flower exchange orders with the assistance of a priority queue.

## Order Book 
- The order book consists of two sides. Namely; the Buy side and the Sell side with ascending and descending order price sorting respectively. However, orders with the same price are sorted according to a priority sequence.
- Output execution report consists of 4 statuses and the order book entries are processed according to these statuses.
- The order book is initially declared empty. Therefore, every new order will be given the status of "New"
- A total of three scenarios are addressed
  1. Most attractive order quantity = opponent order quantity

      _Both orders will be executed as "Fill" orders._
  3. Most attractive order quantity < opponent order quantity
     
      _The new order is executed as a 'Fill' order and the FOS/FBO* is executed as a 'PFill'._
  5. Most attractive order quantity > opponent order quantity
     
      _The FSO/FBO* will be executed as a 'Fill' order, and then the buy/sell order will be executed as a 'PFill'._

*FSO refers to the topmost order of the sell-side (first sell order) and FBO refers to the topmost order of the buy-side (first buy order).
  


