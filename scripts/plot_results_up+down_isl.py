import sqlite3
import networkx as nx
import matplotlib.pyplot as plt

# NODES = [100, 500, 1000]
NODES = [5, 10, 100, 500]

fig, ax = plt.subplots(1, 4, figsize=(25,8))
vmin = 0
vmax = 4275.0
cmap=plt.cm.jet
style = "solid"
weight = 20
node_size = 1800

for n, nodes_number in enumerate(NODES):
    file = "results_isl/General-avg,numberOfNodes="+str(nodes_number)+"-#0.sca"

    conn = sqlite3.connect(file)
    conn.row_factory = sqlite3.Row
    cur = conn.cursor()

    cur.execute("SELECT * FROM scalar WHERE scalarName='%s'" % ("sentPacketsUp:count"))
    rows = cur.fetchall()
    up = {i:rows[i]["scalarValue"] for i in range(16)}

    cur.execute("SELECT * FROM scalar WHERE scalarName='%s'" % ("sentPacketsRight:count"))
    rows = cur.fetchall()
    right = {i:rows[i]["scalarValue"] for i in range(16)}

    cur.execute("SELECT * FROM scalar WHERE scalarName='%s'" % ("sentPacketsDown:count"))
    rows = cur.fetchall()
    down = {i:rows[i]["scalarValue"] for i in range(16)}

    cur.execute("SELECT * FROM scalar WHERE scalarName='%s'" % ("sentPacketsLeft:count"))
    rows = cur.fetchall()
    left = {i:rows[i]["scalarValue"] for i in range(16)}

    G = nx.Graph()

    positions = {
        0: (1,1),
        1: (1,2),
        2: (1,3),
        3: (1,4),
        4: (2, 1),
        5: (2, 2),
        6: (2, 3),
        7: (2, 4),
        8: (3, 1),
        9: (3, 2),
        10: (3, 3),
        11: (3, 4),
        12: (4, 1),
        13: (4, 2),
        14: (4, 3),
        15: (4, 4),
    }

    # verticals
    G.add_edge(0, 1, style=style, value=int(up[0]+down[1]), weight=weight)
    G.add_edge(1, 2, style=style, value=int(up[1]+down[2]), weight=weight)
    G.add_edge(2, 3, style=style, value=int(up[2]+down[3]), weight=weight)
    G.add_edge(4, 5, style=style, value=int(up[4]+down[5]), weight=weight)
    G.add_edge(5, 6, style=style, value=int(up[5]+down[6]), weight=weight)
    G.add_edge(6, 7, style=style, value=int(up[6]+down[7]), weight=weight)
    G.add_edge(8, 9, style=style, value=int(up[8]+down[9]), weight=weight)
    G.add_edge(9, 10, style=style, value=int(up[9]+down[10]), weight=weight)
    G.add_edge(10, 11, style=style, value=int(up[10]+down[11]), weight=weight)
    G.add_edge(12, 13, style=style, value=int(up[12]+down[13]), weight=weight)
    G.add_edge(13, 14, style=style, value=int(up[13]+down[14]), weight=weight)
    G.add_edge(14, 15, style=style, value=int(up[14]+down[15]), weight=weight)

    # horizontals
    G.add_edge(0, 4, style=style, value=int(right[0]+left[4]), weight=weight)
    G.add_edge(1, 5, style=style, value=int(right[1]+left[5]), weight=weight)
    G.add_edge(2, 6, style=style, value=int(right[2]+left[6]), weight=weight)
    G.add_edge(3, 7, style=style, value=int(right[3]+left[7]), weight=weight)
    G.add_edge(7, 11, style=style, value=int(right[7]+left[11]), weight=weight)
    G.add_edge(6, 10, style=style, value=int(right[6]+left[10]), weight=weight)
    G.add_edge(5, 9, style=style, value=int(right[5]+left[9]), weight=weight)
    G.add_edge(4, 8, style=style, value=int(right[4]+left[8]), weight=weight)
    G.add_edge(11, 15, style=style, value=int(right[11]+left[15]), weight=weight)
    G.add_edge(10, 14, style=style, value=int(right[10]+left[14]), weight=weight)
    G.add_edge(9, 13, style=style, value=int(right[9]+left[13]), weight=weight)
    G.add_edge(8, 12, style=style, value=int(right[8]+left[12]), weight=weight)

    edges = G.edges()
    values = [G[u][v]['value'] for u,v in edges]
    edges_values_dict = {(u,v): G[u][v]['value'] for u,v in edges}

    widths = [G[u][v]['weight'] for u,v in edges]
    nx.draw(ax=ax[n],
            G=G,
            pos=positions,
            edge_color=values,
            node_color="white",
            edgecolors="black",
            width=widths,
            with_labels=True,
            node_size=node_size,
            edge_cmap=cmap,
            edge_vmin=vmin,
            edge_vmax=vmax)

    nx.draw_networkx_edge_labels(ax=ax[n], G=G, pos=positions, edge_labels=edges_values_dict)

    ax[n].title.set_text("Number of Nodes = " + str(nodes_number) +
                         "\nTotal packets sent = " + str(int(sum(values))))

#plt.suptitle("uplink + downlink", fontsize=16, x=0.465)
fig.subplots_adjust(right=0.8)
sm = plt.cm.ScalarMappable(cmap=cmap, norm=plt.Normalize(vmin=vmin, vmax=vmax))
cbar_ax = fig.add_axes([0.85, 0.15, 0.02, 0.7])
plt.colorbar(sm, cax=cbar_ax)
plt.savefig("up+down.png")
plt.savefig("up+down.pdf")
