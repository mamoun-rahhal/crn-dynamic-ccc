#include <iostream>
#include <vector>
#include <random>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <string>

const unsigned SLOTS = 30000;
const unsigned M = 19;                  // regular bands
const unsigned UNDERLAY_BAND = 19;      // band 19 is underlay
const unsigned TOTAL_BANDS = 20;        // total including underlay
const unsigned N = 20;                  // total SUs (each CH expects 4 pkts each timeslot)
const unsigned NUM_CHS = 5;
const unsigned SUS_PER_CH = 4;
const unsigned DEMAND = 1;
const double Pmd = 0.0;                 // Perfect sensing
const double Pfa = 0.0;                 // Perfect sensing
const unsigned TINACTIVE = 100 ;
const unsigned MAX_BACKOFF = 512 ;

std::mt19937_64 randEngine{123} ;

enum CHState
{
    SEARCHING,    // looking for initial regular band
    NORMAL,       // working on regular band
    DISPLACED,    // PU appeared, executing backoff algorithm
    ON_UNDERLAY
};

std::string stateToString(CHState state)
{
    switch(state)
    {
    case SEARCHING: return "SEARCHING" ;
    case NORMAL: return "NORMAL" ;
    case DISPLACED: return "DISPLACED" ;
    case ON_UNDERLAY: return "ON_UNDERLAY" ;
    default: return "UNKNOWN";
    }
}

class Band
{
public:
    bool puActive ;
    double probOFFtoON;
    double probONtoOFF;
    Band(double activityProbability):
        puActive(false) ,
        probOFFtoON(1.0/TINACTIVE),   // constant = 0.01
        probONtoOFF((1.0-activityProbability)/(activityProbability*TINACTIVE)) {}

    void updateMarkov()
    {
        std::bernoulli_distribution trans(puActive ? probONtoOFF : probOFFtoON);
        // Coin flip
        bool Switch = trans(randEngine);
        if (Switch) {
            puActive = !puActive;
            // Determine if state should flip this timeslot or not
        }
    }
};

class SecondaryUser
{
public:
    unsigned int id;
    unsigned int chId;  // cluster head ID
    SecondaryUser(unsigned int userId, unsigned int clusterId) :
        id(userId), chId(clusterId) {}
};

class ClusterHead
{
public:
    unsigned int id;
    CHState state;
    unsigned int currentBand;
    unsigned int backoffCounter;

    // new performance metrics
    unsigned fullPacketSlots = 0;      // slots with all 4 packets received
    unsigned totalPacketsReceived = 0 ; // Total packets received
    unsigned totalPacketsLost = 0;     // packets lost due to displacement/underlay
    unsigned outageSlots = 0;          // time in DISPLACED/ON_UNDERLAY states
    unsigned outageEvents = 0 ;
    unsigned underlayAccessAttempts = 0 ;
    unsigned successfulUnderlayAccesses = 0 ;

    ClusterHead(unsigned int chId) :
        id(chId), state(SEARCHING), currentBand(0), backoffCounter(0) {}

    // searching for an empty band when going to underlay
    bool findAvailableRegularBand(const std::vector<Band>& bands, const std::vector<ClusterHead>& allCHs)
    {
        for (unsigned int m = 0; m < M; ++m)
        {
            if (!bands[m].puActive)
            {
                bool bandInUse = false ;
                for (const auto& ch : allCHs)
                {
                    if (ch.id != id && ch.state == NORMAL && ch.currentBand == m)
                    {
                        bandInUse = true;
                        break;
                    }
                }
                if (!bandInUse)
                {
                    currentBand = m;
                    return true;
                }
            }
        }
        return false;
    }

    void resetBackoff()
    {
        backoffCounter = 0;
    }

    void executeBackoff()
    // all of this is explained in the paper
    {
        std::bernoulli_distribution failDist(0.12) ;
        if (failDist(randEngine)) {
            backoffCounter = std::min(backoffCounter * 2, MAX_BACKOFF) ;
        } else {
            std::uniform_int_distribution<unsigned> jitterDist(0, 24);
            backoffCounter = std::min(backoffCounter * 2 + jitterDist(randEngine), MAX_BACKOFF) ;
        }
        if (backoffCounter == 0) backoffCounter = 1 ;
    }

    void initializeBackoff()
    {
        std::uniform_int_distribution<unsigned> initialBackoff(2, 6);
        backoffCounter = initialBackoff(randEngine);
    }

    void updateState(CHState newState, unsigned slot)
    {
        if (state == NORMAL && newState != NORMAL)
        {
            outageEvents++;
        }
        state = newState;
    }
};

void executeClusterSimulation(const std::string& baseFilename, double puProb)
{
    // initializing bands
    std::vector<Band> bands;
    for (unsigned m = 0; m < TOTAL_BANDS; ++m)
    {
        bands.emplace_back(puProb);
    }
    // initialize cluster heads
    std::vector<ClusterHead> clusterHeads;
    for (unsigned ch = 0; ch < NUM_CHS; ++ch)
    {
        clusterHeads.emplace_back(ch);
    }
    // initialize secondary users
    std::vector<SecondaryUser> users;
    for (unsigned n = 0; n < N; ++n)
    {
        unsigned chId = n / SUS_PER_CH;
        users.emplace_back(n, chId);
    }

    // output file
    std::ofstream chStatesFile(baseFilename + "_ch_states.csv") ;
    chStatesFile << "Slot,CH_ID,CurrentBand,State,BackoffCounter\n" ;

    // Progress tracking (this is just added to know how much of simulator is done)
    unsigned progressInterval = SLOTS / 10;
    for (unsigned t = 0; t < SLOTS; ++t)
    {
        if (t % progressInterval == 0)
        {
            std::cout << "   Progress: " << (t * 100 / SLOTS) << "%" << std::endl ;
        }
        // Update band states
        for (auto& band : bands)
        {
            band.updateMarkov();
        }
        // 1) PU activation check and displacement
        for (auto& ch : clusterHeads)
        {
            if (ch.state == NORMAL && bands[ch.currentBand].puActive)
            {
                ch.updateState(DISPLACED, t);
                ch.initializeBackoff() ;
            }
        }
        // 2. CH state handling (all cases of the system)
        for (auto& ch : clusterHeads)
        {
            switch (ch.state)
            {
            case DISPLACED:
                if (ch.backoffCounter > 0)
                {
                    ch.backoffCounter--;
                } else
                {
                    ch.underlayAccessAttempts++ ;
                    double collisionProb = 0.008 + 0.035 * puProb;
                    std::bernoulli_distribution collisionDist(collisionProb);
                    if (!collisionDist(randEngine))
                    {
                        ch.updateState(ON_UNDERLAY, t);
                        ch.currentBand = UNDERLAY_BAND;
                        ch.resetBackoff();
                        ch.successfulUnderlayAccesses++;
                    } else {
                        ch.executeBackoff();
                    }
                }
                break;
            case ON_UNDERLAY:
                if (ch.findAvailableRegularBand(bands, clusterHeads))
                {
                    ch.updateState(NORMAL, t);
                }
                break;
            case SEARCHING:
                if (ch.findAvailableRegularBand(bands, clusterHeads))
                {
                    ch.updateState(NORMAL, t);
                }
                break;
            case NORMAL:
                // already done above
                break ;
            }
        }
        // 3. Packet transmission and reception
        for (auto& ch : clusterHeads)
        {
            if (ch.state == NORMAL)
            {
                // SU pkts always received successfully in NORMAL state
                ch.totalPacketsReceived += SUS_PER_CH;
                if (SUS_PER_CH == 4)
                {  // all packets received
                    ch.fullPacketSlots++;
                }
            } else
            {
                // no packets received in displaced/underlay/searching states
                // SUs cannot send packets to CH when it's on underlay or displaced
                ch.totalPacketsLost += SUS_PER_CH;
                ch.outageSlots++;
            }
        }
        // record CH states for every timeslot
        for (const auto& ch : clusterHeads)
        {
            chStatesFile << t << "," << ch.id << ","
                         << ch.currentBand << ","
                         << stateToString(ch.state) << ","
                         << ch.backoffCounter << "\n";
        }
    }
    chStatesFile.close();

    // summary file
    std::ofstream summaryFile(baseFilename + "_summary.csv");
    summaryFile << "Metric,Value\n";
    summaryFile << "Total_Slots," << SLOTS << "\n";
    summaryFile << "PU_Probability," << puProb << "\n";
    summaryFile << "Cluster_Heads," << NUM_CHS << "\n";
    summaryFile << "Secondary_Users," << N << "\n";
    summaryFile << "Regular_Bands," << M << "\n";
    summaryFile << "Underlay_Band," << UNDERLAY_BAND << "\n";
    // per-CH metrics
    for (const auto& ch : clusterHeads)
    {
        double pdr = static_cast<double>(ch.fullPacketSlots) / SLOTS ;
        // calculate packet lost percentage
        const unsigned TOTAL_PACKETS = SLOTS * SUS_PER_CH; // 30000 * 4 = 120000
        double packetLostPercentage = static_cast<double>(ch.totalPacketsLost) / TOTAL_PACKETS ;
        double avgOutageDuration = ch.outageEvents > 0 ?
                                       static_cast<double>(ch.outageSlots) / ch.outageEvents : 0.0 ;
        summaryFile << "CH" << ch.id << "_PDR," << pdr << "\n";
        summaryFile << "CH" << ch.id << "_Packets_Lost," << (ch.totalPacketsLost) << "\n";
        summaryFile << "CH" << ch.id << "_Packet_Lost_Percentage," << packetLostPercentage << "\n";
        summaryFile << "CH" << ch.id << "_Total_Outage_Slots," << ch.outageSlots << "\n";
        summaryFile << "CH" << ch.id << "_Avg_Outage_Duration," << avgOutageDuration << "\n";
        summaryFile << "CH" << ch.id << "_Underlay_Access_Success_Rate,"
                    << (ch.underlayAccessAttempts > 0 ?
                            static_cast<double>(ch.successfulUnderlayAccesses) / ch.underlayAccessAttempts : 0)
                    << "\n";
    }
    summaryFile.close();
    std::cout << "  Simulation completed for PU probability: " << puProb << std::endl;
}

int main()
{
    std::vector<double> puProbs = {0.3, 0.5, 0.7} ;
    for (double prob : puProbs)
    {
        std::cout << "Running simulation with PU probability: " << prob << std::endl;
        std::string filename = "packet_based_sim_pu_" + std::to_string(int(prob*100));
        executeClusterSimulation(filename, prob);
        std::cout << std::endl;
    }
    return 0;
}
