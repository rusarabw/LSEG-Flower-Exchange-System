/*
    LSEG - Flower Exchange System V2
        ** Rusara Wimalasena
        ** Senuri Wickramasinghe
*/
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <iterator>
#include <chrono>
#include <queue>

using namespace std; 

// Structure to store an order
class Order {
public: 
    static int curOrderId;
    string orderId;
    string clientOrderId;
    string instrument;
    int side;
    double price;
    int qty;
    int remQty;
    int status;
    int priority;
    string reason;
    Order() {}
    Order(string *,string *,string *,string *,string *);
    void checkOrder(string *,string *,string *,string *,string *);  // check whether an order is valid or not
    bool checkValidity();                                           // check order status
    void execute(ofstream&, int);                                   // Write to the execution_rep file
    void execute(ofstream&);                                        // Overloaded execute for default quantity
};

/*
    Comparator for priority_queue
    * if two orders have the same price, then the priority will be the tie-breaker
    * if the orders are buy orders, then the order with the higher price will get higher priority
    * if the orders are sell orders, then the order with the lower price will get lower priority
*/
struct Compare {
    bool operator() (Order ord1, Order ord2) {
        if(ord1.price == ord2.price) {
            return ord1.priority < ord2.priority;
        }
        if(ord1.side == 1) {
            return ord1.price < ord2.price;
        } else {
            return ord1.price > ord2.price;
        }
    }
};

string instruments[5] = {"Rose","Lavender","Lotus","Tulip","Orchid"};   
int Order::curOrderId = 1;

int main(void) {
    // priority_queues for each side of all the instruments
    priority_queue<Order, vector<Order>, Compare> roseBuy;
    priority_queue<Order, vector<Order>, Compare> roseSell;
    priority_queue<Order, vector<Order>, Compare> lavenderBuy;
    priority_queue<Order, vector<Order>, Compare> lavenderSell;
    priority_queue<Order, vector<Order>, Compare> lotusBuy;
    priority_queue<Order, vector<Order>, Compare> lotusSell;
    priority_queue<Order, vector<Order>, Compare> tulipBuy;
    priority_queue<Order, vector<Order>, Compare> tulipSell;
    priority_queue<Order, vector<Order>, Compare> orchidBuy;
    priority_queue<Order, vector<Order>, Compare> orchidSell;

    // abstraction of priority_queues
    priority_queue<Order, vector<Order>, Compare>* roseOB[2] = {&roseBuy, &roseSell};
    priority_queue<Order, vector<Order>, Compare>* lavenderOB[2] = {&lavenderBuy, &lavenderSell};
    priority_queue<Order, vector<Order>, Compare>* lotusOB[2] = {&lotusBuy, &lotusSell};
    priority_queue<Order, vector<Order>, Compare>* tulipOB[2] = {&tulipBuy, &tulipSell};
    priority_queue<Order, vector<Order>, Compare>* orchidOB[2] = {&orchidBuy, &orchidSell};

    priority_queue<Order, vector<Order>, Compare>** orderBooks[5] = {roseOB, lavenderOB, lotusOB, tulipOB, orchidOB};

    // I/O file handling
    ifstream fin;
    fin.open("order.csv", ios::in);
    ofstream fout;
    fout.open("execution_rep.csv", ios::out);
    fout << "Client Order ID,Order ID,Instrument,Side,Price,Quantity,Status,Reason,Transaction Time\n";

    // Auxilaries for file parsing
    vector<string> row;
    string line, word, temp;   

    // Parsing order.csv file
    int count = 0;
    while(getline(fin, line)) {
        if(++count == 1) 
            continue;
        row.clear();
        stringstream s(line);
        while(getline(s, word, ',')) {
            row.push_back(word);
        }
        // Constructing a new order 
        Order newOrder(&row[0], &row[1], &row[2], &row[3], &row[4]);

        if(newOrder.checkValidity()) {
            // Get the index of the instrument 
            int index = (int)(find(begin(instruments), end(instruments), newOrder.instrument) - begin(instruments));
            priority_queue<Order, vector<Order>, Compare>** orderBook = orderBooks[index];

            if(newOrder.side == 1) {
                /* 
                    If the sell side is not empty 
                   and the most attractive sell order can fulfill the buyer's buy order 
                */
                while(!(orderBook[1]->empty()) && ((orderBook[1]->top()).price <= newOrder.price)) {
                    Order topOrder = (orderBook[1]->top());
                    if(newOrder.remQty == topOrder.remQty) {             // most attractive sell order quantity = buy order quantity
                        newOrder.status = 2;
                        topOrder.status = 2;
                        newOrder.price = topOrder.price;
                        newOrder.execute(fout);
                        topOrder.execute(fout);
                        newOrder.remQty = 0;
                        topOrder.remQty = 0;
                        orderBook[1]->pop();
                        break;
                    } else if(newOrder.remQty > topOrder.remQty) {       // most attractive sell order quantity < buy order quantity
                        double temp = newOrder.price;
                        newOrder.status = 3;
                        topOrder.status = 2;
                        newOrder.price = topOrder.price;
                        newOrder.execute(fout, topOrder.remQty);
                        topOrder.execute(fout);
                        newOrder.remQty = newOrder.remQty - topOrder.remQty;
                        topOrder.remQty = 0;
                        newOrder.price = temp;
                        orderBook[1]->pop();
                    } else {                                                    // most attractive sell order quantity > buy order quantity
                        newOrder.status = 2;
                        topOrder.status = 3;
                        newOrder.price = topOrder.price;
                        newOrder.execute(fout);
                        topOrder.execute(fout, newOrder.remQty);
                        topOrder.remQty = topOrder.remQty - newOrder.remQty;
                        topOrder.priority++;
                        orderBook[1]->pop();
                        orderBook[1]->push(topOrder);
                        newOrder.remQty = 0;
                        break;
                    }
                }

                // Depicts a 'New' order being put into the order book
                if(newOrder.status == 0) {
                    newOrder.execute(fout);
                }

                // If the aggressive order is not fully executed, then put it into the order book 
                if(newOrder.remQty > 0.0) {
                    orderBook[0]->push(newOrder);
                }

            // Sell Side
            } else if(newOrder.side == 2) {
                /* 
                    If the buy side is not empty 
                    and the most attractive buy order can fulfill the seller's sell order
                */
                while(!(orderBook[0]->empty()) && ((orderBook[0]->top()).price >= newOrder.price)) {
                    Order topOrder = orderBook[0]->top();
                    if(newOrder.remQty == topOrder.remQty) {                 // most attractive buy order quantity = sell order quantity
                        newOrder.status = 2;
                        topOrder.status = 2;
                        newOrder.price = topOrder.price; 
                        newOrder.execute(fout);
                        topOrder.execute(fout);
                        newOrder.remQty = 0;
                        topOrder.remQty = 0;
                        orderBook[0]->pop();
                        break;
                    } else if(newOrder.remQty > topOrder.remQty) {           // most attractive buy order quantity < sell order quantity
                        double temp2 = newOrder.price;
                        newOrder.status = 3;
                        topOrder.status = 2;
                        newOrder.price = topOrder.price; 
                        newOrder.execute(fout,topOrder.remQty);
                        newOrder.remQty = newOrder.remQty - topOrder.remQty;
                        topOrder.execute(fout);
                        topOrder.remQty = 0;
                        newOrder.price = temp2;
                        orderBook[0]->pop();
                    } else {                                                        // most attractive buy order quantity > sell order quantity
                        newOrder.status = 2;
                        topOrder.status = 3;
                        newOrder.price = topOrder.price;
                        newOrder.execute(fout);
                        topOrder.execute(fout, newOrder.remQty);
                        topOrder.remQty = topOrder.remQty - newOrder.remQty;
                        topOrder.priority++;
                        orderBook[0]->pop();
                        orderBook[0]->push(topOrder);
                        newOrder.remQty = 0;
                        break;
                    }
                }

                // Depicts a 'New' order being put into the order book
                if(newOrder.status == 0) {
                    newOrder.execute(fout);
                }

                // If the aggressive order is not fully executed, then put it into the order book 
                if(newOrder.remQty > 0.0) {
                    orderBook[1]->push(newOrder);
                }
            }
        } else {
            newOrder.execute(fout);
        }
    }

    return 0;
}

Order::Order(string *_clientOrderId, string *_instrument, string *_side, string *_qty, string *_price) {
    orderId = "ord" + to_string(curOrderId++);
    checkOrder(_clientOrderId, _instrument, _side, _qty, _price);
    clientOrderId = *_clientOrderId;
    instrument = *_instrument;
    side = stoi(*_side);
    price = stod(*_price);
    qty = stoi(*_qty);
    remQty = qty;
    priority = 1;
}

void Order::checkOrder(string *_clientOrderId, string *_instrument, string *_side, string *_qty, string *_price) {
    if(!((*_clientOrderId).length() && (*_instrument).length() && (*_side).length() && (*_price).length() && (*_qty).length())) {
        reason = "Invalid fields";
        status = 1;    
        return;
    }
    if(find(begin(instruments), end(instruments), *_instrument) == end(instruments)) {
        reason = "Invalid instrument";
        status = 1;   
        return;
    }
    int s = stoi(*_side);
    if(!(s == 1 || s == 2)) {
        reason = "Invalid side";
        status = 1;   
        return;  
    }
    double p = stod(*_price);
    if(p <= 0.0) {
        reason = "Invalid price";
        status = 1;  
        return;
    }
    int q = stoi(*_qty);
    if(q%10 != 0 || q < 10 || q > 1000) {
        reason = "Invalid size";
        status = 1;  
        return;   
    }
    reason = "";
    status = 0;
}   

bool Order::checkValidity() {
    if(status == 1)
        return false;
    return true;
}

void Order::execute(ofstream &fout, int execQty) {
    auto now = chrono::system_clock::now();
    auto now_ms = chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    time_t t = chrono::system_clock::to_time_t(now);
    tm* tm = localtime(&t);
    string stat = "";
    if(status == 0) stat = "New";
    else if(status == 1) stat = "Reject";
    else if(status == 2) stat = "Fill";
    else if(status == 3) stat = "PFill";
    fout << clientOrderId << "," << orderId << "," << instrument << "," << side << "," << price << "," << execQty << "," << stat << "," 
    << reason << "," << put_time(tm, "%Y.%m.%d-%H.%M.%S") << "." << setfill('0') << setw(3) << to_string(now_ms.count()) << "\n"; 
}

void Order::execute(ofstream &fout) {
    execute(fout, remQty);
}
