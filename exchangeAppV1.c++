/*
    LSEG - Flower Exchange System
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


void insertOB(vector<Order>& ,Order ,int );     // Insert order into the order book
string instruments[5] = {"Rose","Lavender","Lotus","Tulip","Orchid"};   
int Order::curOrderId = 1;

int main(void) {
    // vectors to represent order books for each instrument
    vector<vector<Order> > obRose(2);
    vector<vector<Order> > obLavender(2);
    vector<vector<Order> > obLotus(2);
    vector<vector<Order> > obTulip(2);
    vector<vector<Order> > obOrchid(2);

    // vector to keep track of all the order books (in the same order as in 'instruments')
    vector<vector<Order> > orderBooks[5] = {obRose, obLavender, obLotus, obTulip, obOrchid};

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

        // Check whether the order is valid or not
        if(newOrder.checkValidity()) {
            // Get the index of the instrument 
            int index = (int)(find(begin(instruments), end(instruments), newOrder.instrument) - begin(instruments));
            vector<vector<Order> > orderBook = orderBooks[index];

            // Buy Side
            if(newOrder.side == 1) {
                /* If the sell side is not empty 
                   and the most attractive sell order can fulfill the buyer's buy order */
                while(!(orderBook[1].empty()) && (orderBook[1][0].price <= newOrder.price)) {
                    if(newOrder.remQty == orderBook[1][0].remQty) {             // most attractive sell order quantity = buy order quantity
                        newOrder.status = 2;
                        orderBook[1][0].status = 2;
                        newOrder.price = orderBook[1][0].price;
                        newOrder.execute(fout);
                        orderBook[1][0].execute(fout);
                        newOrder.remQty = 0;
                        orderBook[1][0].remQty = 0;
                        orderBook[1].erase(begin(orderBook[1]));
                        break;
                    } else if(newOrder.remQty > orderBook[1][0].remQty) {       // most attractive sell order quantity < buy order quantity
                        double temp = newOrder.price;
                        newOrder.status = 3;
                        orderBook[1][0].status = 2;
                        newOrder.price = orderBook[1][0].price;
                        newOrder.execute(fout, orderBook[1][0].remQty);
                        orderBook[1][0].execute(fout);
                        newOrder.remQty = newOrder.remQty - orderBook[1][0].remQty;
                        orderBook[1][0].remQty = 0;
                        newOrder.price = temp;
                        orderBook[1].erase(begin(orderBook[1]));
                    } else {                                                    // most attractive sell order quantity > buy order quantity
                        newOrder.status = 2;
                        orderBook[1][0].status = 3;
                        newOrder.price = orderBook[1][0].price;
                        newOrder.execute(fout);
                        orderBook[1][0].execute(fout, newOrder.remQty);
                        orderBook[1][0].remQty = orderBook[1][0].remQty - newOrder.remQty;
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
                    insertOB(orderBook[0], newOrder, 1);
                }

            // Sell Side
            } else if(newOrder.side == 2) {
                /* If the buy side is not empty 
                   and the most attractive buy order can fulfill the seller's sell order */
                while(!(orderBook[0].empty()) && (orderBook[0][0].price >= newOrder.price)) {
                    if(newOrder.remQty == orderBook[0][0].remQty) {                 // most attractive buy order quantity = sell order quantity
                        newOrder.status = 2;
                        orderBook[0][0].status = 2;
                        newOrder.price = orderBook[0][0].price; 
                        newOrder.execute(fout);
                        orderBook[0][0].execute(fout);
                        newOrder.remQty = 0;
                        orderBook[0][0].remQty = 0;
                        orderBook[0].erase(begin(orderBook[0]));
                        break;
                    } else if(newOrder.remQty > orderBook[0][0].remQty) {           // most attractive buy order quantity < sell order quantity
                        double temp2 = newOrder.price;
                        newOrder.status = 3;
                        orderBook[0][0].status = 2;
                        newOrder.price = orderBook[0][0].price; 
                        newOrder.execute(fout,orderBook[0][0].remQty);
                        newOrder.remQty = newOrder.remQty - orderBook[0][0].remQty;
                        orderBook[0][0].execute(fout);
                        orderBook[0][0].remQty = 0;
                        newOrder.price = temp2;
                        orderBook[0].erase(begin(orderBook[0]));
                    } else {                                                        // most attractive buy order quantity > sell order quantity
                        newOrder.status = 2;
                        orderBook[0][0].status = 3;
                        newOrder.price = orderBook[0][0].price;
                        newOrder.execute(fout);
                        orderBook[0][0].execute(fout, newOrder.remQty);
                        orderBook[0][0].remQty = orderBook[0][0].remQty - newOrder.remQty;
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
                    insertOB(orderBook[1], newOrder, 2);
                }
            }

            // Save the context of the current order book
            orderBooks[index] = orderBook;
        } else {
            // Execute the order if it's invalid
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
    auto now_ms = chrono::time_point_cast<chrono::milliseconds>(now);
    auto value = now_ms.time_since_epoch();
    long duration = value.count();
    time_t t = chrono::system_clock::to_time_t(now);
    tm* tm = localtime(&t);
    string stat = "";
    if(status == 0) stat = "New";
    else if(status == 1) stat = "Reject";
    else if(status == 2) stat = "Fill";
    else if(status == 3) stat = "PFill";
    fout << clientOrderId << "," << orderId << "," << instrument << "," << side << "," << price << "," << execQty << "," << stat << "," 
    << reason << "," << put_time(tm, "%Y.%m.%d-%H.%M.%S") << "." << setfill('0') << setw(3) << duration%1000 << "\n"; 
}

void Order::execute(ofstream &fout) {
    execute(fout, remQty);
}

void insertOB(vector<Order>& vect, Order newOrder, int side) {
    if(vect.empty()) {
        vect.push_back(newOrder);
        return;
    }
    auto it = begin(vect);
    while(it != end(vect)) {
        if((it->price < newOrder.price && side == 1) || (it->price > newOrder.price && side == 2))
            break;
        ++it;
    }
    if(it == end(vect)) {
        vect.push_back(newOrder);
    } else {
        if((it != begin(vect)) && (it-1)->price == newOrder.price) {
            newOrder.priority = ((it-1)->priority) + 1;
        }
        vect.insert(it, newOrder);
    }
}